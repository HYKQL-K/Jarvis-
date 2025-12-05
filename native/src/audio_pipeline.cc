#include "jarvis/audio_pipeline.h"

#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#if defined(JARVIS_AUDIO_USE_WEBRTC)
#include "modules/audio_processing/include/audio_processing.h"
#endif

namespace {

struct AudioPipelineContext {
#if defined(JARVIS_AUDIO_USE_WEBRTC)
    std::unique_ptr<webrtc::AudioProcessing> apm;
    webrtc::StreamConfig stream_config;
#endif
    int channels = 0;
    int frame_samples = 0;
    std::vector<std::vector<int16_t>> channel_buffers;
    std::vector<const int16_t*> input_ptrs;
    std::vector<int16_t*> output_ptrs;
    float energy_threshold = 200.0f; // simple stub VAD threshold (hykql原创)
};

std::mutex g_mutex;
std::unique_ptr<AudioPipelineContext> g_ctx;

int ValidateParams(const jarvis_audio_params* params) {
    if (!params) return -1;
    if (params->sample_rate_hz <= 0 || params->num_channels <= 0 || params->frame_samples <= 0) return -2;
    if (params->num_channels > 8) return -3;
    return 0;
}

float ComputeRms(const int16_t* pcm, int n) {
    double acc = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = static_cast<double>(pcm[i]);
        acc += s * s;
    }
    return static_cast<float>(std::sqrt(acc / std::max(1, n)));
}

}  // namespace

extern "C" int jarvis_audio_init(const jarvis_audio_params* params) {
    const int validation = ValidateParams(params);
    if (validation != 0) {
        return validation;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    g_ctx = std::make_unique<AudioPipelineContext>();
    g_ctx->channels = params->num_channels;
    g_ctx->frame_samples = params->frame_samples;

#if defined(JARVIS_AUDIO_USE_WEBRTC)
    g_ctx->stream_config = webrtc::StreamConfig(params->sample_rate_hz, params->num_channels);
    auto apm = webrtc::AudioProcessingBuilder().Create();
    if (!apm) {
        g_ctx.reset();
        return -10;
    }
    webrtc::AudioProcessing::Config config;
    config.echo_canceller.enabled = params->enable_aec;
    config.gain_controller1.enabled = params->enable_agc;
    config.gain_controller1.mode = webrtc::AudioProcessing::Config::GainController1::kAdaptiveDigital;
    config.noise_suppression.enabled = params->enable_ns;
    config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;
    config.voice_detection.enabled = params->enable_vad;
    apm->ApplyConfig(config);
    g_ctx->apm = std::move(apm);
#else
    // Stub: set an energy threshold that can be tuned by frame size. (hykql原创)
    g_ctx->energy_threshold = params->frame_samples * 4.0f;
#endif

    g_ctx->channel_buffers.resize(params->num_channels);
    g_ctx->input_ptrs.resize(params->num_channels);
    g_ctx->output_ptrs.resize(params->num_channels);

    return 0;
}

extern "C" int jarvis_audio_process(int16_t* pcm, int n, bool* is_speech) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_ctx) {
        return -20;
    }

    const int expected = g_ctx->frame_samples * g_ctx->channels;
    if (n != expected) {
        return -21;
    }

#if defined(JARVIS_AUDIO_USE_WEBRTC)
    if (!g_ctx->apm) return -22;
    for (int ch = 0; ch < g_ctx->channels; ++ch) {
        auto& buf = g_ctx->channel_buffers[ch];
        buf.resize(g_ctx->frame_samples);
        for (int i = 0; i < g_ctx->frame_samples; ++i) {
            buf[i] = pcm[i * g_ctx->channels + ch];
        }
        g_ctx->input_ptrs[ch] = buf.data();
        g_ctx->output_ptrs[ch] = buf.data();
    }

    const int err = g_ctx->apm->ProcessStream(
        g_ctx->input_ptrs.data(),
        g_ctx->stream_config,
        g_ctx->stream_config,
        g_ctx->output_ptrs.data());

    if (err != webrtc::AudioProcessing::kNoError) {
        return err;
    }

    for (int ch = 0; ch < g_ctx->channels; ++ch) {
        const auto& buf = g_ctx->channel_buffers[ch];
        for (int i = 0; i < g_ctx->frame_samples; ++i) {
            pcm[i * g_ctx->channels + ch] = buf[i];
        }
    }

    if (is_speech) {
        bool voice = false;
        const auto stats = g_ctx->apm->GetStatistics();
        if (stats.voice_detected.has_value()) {
            voice = *stats.voice_detected;
        } else if (g_ctx->apm->voice_detection()) {
            voice = g_ctx->apm->voice_detection()->stream_has_voice();
        }
        *is_speech = voice;
    }
#else
    // Stub VAD: energy-based. (hykql原创)
    if (is_speech) {
        const float rms = ComputeRms(pcm, n);
        *is_speech = rms >= g_ctx->energy_threshold;
    }
#endif

    return 0;
}

extern "C" void jarvis_audio_close(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_ctx.reset();
}
