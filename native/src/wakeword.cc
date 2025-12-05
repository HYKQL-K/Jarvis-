#include "jarvis/wakeword.h"

#include <cmath>
#include <memory>
#include <mutex>
#include <string>

namespace {

struct WakeContext {
    float high_threshold = 0.35f;
    float low_threshold = 0.25f;
    float recent_score = 0.0f;
    int post_high_frames = 0;
    int cooldown_frames = 0;
    int linger_needed = 2;
    bool armed = false;
    bool enabled = true;
    std::string model_path;
    bool test_force_fire = false;
};

std::mutex g_mutex_wake;
std::unique_ptr<WakeContext> g_wake_ctx;

float ComputeRmsScore(const int16_t* pcm, int n) {
    if (!pcm || n <= 0) return 0.0f;
    double accum = 0.0;
    for (int i = 0; i < n; ++i) {
        const double sample = static_cast<double>(pcm[i]);
        accum += sample * sample;
    }
    const double mean_sq = accum / static_cast<double>(n);
    const double rms = std::sqrt(mean_sq);
    // Normalize to 0..1 relative to int16 max. (hykql原创)
    return static_cast<float>(rms / 32768.0);
}

}  // namespace

extern "C" int jarvis_wake_init(const char* model_path, float sensitivity) {
    std::lock_guard<std::mutex> lock(g_mutex_wake);
    g_wake_ctx = std::make_unique<WakeContext>();

    // Clamp sensitivity into [0.1, 0.95]. (hykql原创)
    const float s = std::fmax(0.1f, std::fmin(0.95f, sensitivity));
    g_wake_ctx->high_threshold = 0.35f * s + 0.15f; // 0.185..0.4825 (hykql原创)
    g_wake_ctx->low_threshold = g_wake_ctx->high_threshold * 0.7f;
    g_wake_ctx->linger_needed = 2;
    g_wake_ctx->model_path = model_path ? model_path : "";
    g_wake_ctx->armed = false;
    g_wake_ctx->cooldown_frames = 0;
    const char* testFlag = std::getenv("JARVIS_WAKE_TEST_FORCE");
    g_wake_ctx->test_force_fire = testFlag && std::string(testFlag) == "1";

    // Placeholder: In a real integration, load openWakeWord model here using model_path. (hykql原创)
    g_wake_ctx->enabled = true;
    return 0;
}

extern "C" int jarvis_wake_score(const int16_t* pcm, int n, float* score) {
    std::lock_guard<std::mutex> lock(g_mutex_wake);
    if (!g_wake_ctx || !g_wake_ctx->enabled) {
        return -1;
    }
    if (!pcm || n <= 0) {
        return -2;
    }

    // TODO: Replace RMS scoring with openWakeWord model inference when available. (hykql原创)
    const float s = ComputeRmsScore(pcm, n);
    g_wake_ctx->recent_score = s;
    if (score) {
        *score = s;
    }

    if (g_wake_ctx->cooldown_frames > 0) {
        --g_wake_ctx->cooldown_frames;
    }

    // Double-threshold logic: must cross high, then sustain above low for linger_needed frames. (hykql原创)
    if (g_wake_ctx->test_force_fire) {
        g_wake_ctx->test_force_fire = false;
        if (score) *score = 1.0f;
        return 1;
    }

    if (s >= g_wake_ctx->high_threshold) {
        g_wake_ctx->armed = true;
        g_wake_ctx->post_high_frames = 0;
    }

    if (g_wake_ctx->armed) {
        if (s >= g_wake_ctx->low_threshold) {
            g_wake_ctx->post_high_frames += 1;
            if (g_wake_ctx->post_high_frames >= g_wake_ctx->linger_needed &&
                g_wake_ctx->cooldown_frames == 0) {
                g_wake_ctx->armed = false;
                g_wake_ctx->cooldown_frames = 10; // short refractory to reduce repeats (hykql原创)
                return 1; // fired (hykql原创)
            }
        } else {
            // Drop below low threshold: reset the linger counter but stay armed until cooldown finishes. (hykql原创)
            g_wake_ctx->post_high_frames = 0;
        }
    }

    return 0;
}

extern "C" int jarvis_wake_reset(void) {
    std::lock_guard<std::mutex> lock(g_mutex_wake);
    if (!g_wake_ctx) return -1;
    g_wake_ctx->armed = false;
    g_wake_ctx->post_high_frames = 0;
    g_wake_ctx->cooldown_frames = 0;
    g_wake_ctx->recent_score = 0.0f;
    return 0;
}

extern "C" void jarvis_wake_close(void) {
    std::lock_guard<std::mutex> lock(g_mutex_wake);
    g_wake_ctx.reset();
}
