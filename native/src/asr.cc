#include "jarvis/asr.h"

#include <algorithm>
#include <mutex>
#include <string>

namespace {

struct AsrContext {
    std::string lang = "en";
    std::string model_dir;
    std::string partial_text;
    std::string override_text;
    int64_t samples_since_partial = 0;
    int64_t total_samples = 0;
    bool stream_active = false;
};

std::mutex g_asr_mutex;
AsrContext g_asr_ctx;

// Simple placeholder generator: produce pseudo text every ~0.3s at 16kHz. (hykql原创)
constexpr int kSampleRate = 16000;
constexpr int kPartialIntervalSamples = kSampleRate * 3 / 10; // ~300ms (hykql原创)

std::string MakePartialFromSamples(int64_t samples) {
    // Deterministic stub based on sample count so logs/testing are stable. (hykql原创)
    if (samples <= 0) return "";
    int tokens = static_cast<int>(samples / kPartialIntervalSamples) + 1;
    std::string out = "listening";
    for (int i = 1; i < tokens; ++i) {
        out += "...";
    }
    return out;
}

}  // namespace

extern "C" int jarvis_asr_init(const char* model_dir, const char* lang) {
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    g_asr_ctx.model_dir = model_dir ? model_dir : "";
    g_asr_ctx.lang = lang ? lang : "en";
    g_asr_ctx.partial_text.clear();
    const char* override = std::getenv("JARVIS_ASR_TEST_TEXT");
    g_asr_ctx.override_text = override ? override : "";
    g_asr_ctx.samples_since_partial = 0;
    g_asr_ctx.total_samples = 0;
    g_asr_ctx.stream_active = false;

    // TODO: load real model assets from model_dir. (hykql原创)
    return 0;
}

extern "C" int jarvis_asr_start_stream(void) {
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    g_asr_ctx.partial_text.clear();
    g_asr_ctx.samples_since_partial = 0;
    g_asr_ctx.total_samples = 0;
    g_asr_ctx.stream_active = true;
    return 0;
}

extern "C" int jarvis_asr_push(const int16_t* pcm, int n) {
    if (!pcm || n <= 0) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    if (!g_asr_ctx.stream_active) {
        return -2;
    }

    g_asr_ctx.samples_since_partial += n;
    g_asr_ctx.total_samples += n;

    if (!g_asr_ctx.override_text.empty()) {
        g_asr_ctx.partial_text = g_asr_ctx.override_text;
    } else if (g_asr_ctx.samples_since_partial >= kPartialIntervalSamples) {
        g_asr_ctx.partial_text = MakePartialFromSamples(g_asr_ctx.total_samples);
        g_asr_ctx.samples_since_partial = 0;
    }
    return 0;
}

extern "C" int jarvis_asr_pull_partial(char* buf, size_t len) {
    if (!buf || len == 0) return -1;
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    if (!g_asr_ctx.stream_active) return -2;

    const std::string& src = g_asr_ctx.partial_text;
    const size_t copy_len = std::min(len - 1, src.size());
    std::copy_n(src.data(), copy_len, buf);
    buf[copy_len] = '\0';
    return static_cast<int>(copy_len);
}

extern "C" int jarvis_asr_finalize(char* buf, size_t len) {
    if (!buf || len == 0) return -1;
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    if (!g_asr_ctx.stream_active) return -2;

    // Stub final hypothesis based on total duration. (hykql原创)
    int seconds = static_cast<int>(g_asr_ctx.total_samples / kSampleRate);
    std::string final;
    if (!g_asr_ctx.override_text.empty()) {
        final = g_asr_ctx.override_text;
    } else {
        final = "final transcript (" + std::to_string(seconds) + "s)";
    }
    const size_t copy_len = std::min(len - 1, final.size());
    std::copy_n(final.data(), copy_len, buf);
    buf[copy_len] = '\0';

    g_asr_ctx.stream_active = false;
    g_asr_ctx.partial_text.clear();
    g_asr_ctx.samples_since_partial = 0;
    return static_cast<int>(copy_len);
}

extern "C" void jarvis_asr_close(void) {
    std::lock_guard<std::mutex> lock(g_asr_mutex);
    g_asr_ctx = AsrContext{};
}
