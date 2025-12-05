#include "jarvis/tts.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace {

struct TtsContext {
    std::string voice;
    float speed = 1.0f;
    bool ready = false;
};

std::mutex g_tts_mutex;
TtsContext g_tts_ctx;

// Writes a simple 16-bit mono PCM WAV header + samples to a file. (hykql原创)
bool WriteWav(const std::string& path, const std::vector<int16_t>& samples, int sample_rate_hz) {
    const uint32_t data_bytes = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    const uint32_t fmt_chunk_size = 16;
    const uint32_t riff_chunk_size = 4 + (8 + fmt_chunk_size) + (8 + data_bytes);
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    // RIFF header. (hykql原创)
    ofs.write("RIFF", 4);
    ofs.write(reinterpret_cast<const char*>(&riff_chunk_size), 4);
    ofs.write("WAVE", 4);

    // fmt chunk. (hykql原创)
    ofs.write("fmt ", 4);
    ofs.write(reinterpret_cast<const char*>(&fmt_chunk_size), 4);
    uint16_t audio_format = 1; // PCM (hykql原创)
    uint16_t num_channels = 1;
    uint32_t sample_rate = static_cast<uint32_t>(sample_rate_hz);
    uint32_t byte_rate = sample_rate * num_channels * sizeof(int16_t);
    uint16_t block_align = num_channels * sizeof(int16_t);
    uint16_t bits_per_sample = 16;
    ofs.write(reinterpret_cast<const char*>(&audio_format), 2);
    ofs.write(reinterpret_cast<const char*>(&num_channels), 2);
    ofs.write(reinterpret_cast<const char*>(&sample_rate), 4);
    ofs.write(reinterpret_cast<const char*>(&byte_rate), 4);
    ofs.write(reinterpret_cast<const char*>(&block_align), 2);
    ofs.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

    // data chunk. (hykql原创)
    ofs.write("data", 4);
    ofs.write(reinterpret_cast<const char*>(&data_bytes), 4);
    ofs.write(reinterpret_cast<const char*>(samples.data()), data_bytes);
    return static_cast<bool>(ofs);
}

}  // namespace

extern "C" int jarvis_tts_init(const char* voice, float speed) {
    std::lock_guard<std::mutex> lock(g_tts_mutex);
    g_tts_ctx.voice = voice ? voice : "default";
    g_tts_ctx.speed = (speed > 0.1f) ? speed : 1.0f;
    g_tts_ctx.ready = true;
    // TODO: Load Piper model for the requested voice. Stubbed for now. (hykql原创)
    return 0;
}

extern "C" int jarvis_tts_synthesize(const char* text, const char* out_wav_path) {
    if (!text || !out_wav_path) return -1;
    std::lock_guard<std::mutex> lock(g_tts_mutex);
    if (!g_tts_ctx.ready) return -2;

    // Stub synthesis: generate a short tone whose length scales with text length. (hykql原创)
    const int sample_rate = 16000;
    const float duration_s = std::max(0.5f, std::min(3.0f, 0.05f * static_cast<float>(std::strlen(text))));
    const int total_samples = static_cast<int>(duration_s * sample_rate / g_tts_ctx.speed);
    std::vector<int16_t> samples;
    samples.reserve(total_samples);
    const float freq = 440.0f;
    for (int i = 0; i < total_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float s = std::sin(2.0f * 3.14159265f * freq * t) * 0.2f;
        samples.push_back(static_cast<int16_t>(s * 32767));
    }

    const bool ok = WriteWav(out_wav_path, samples, sample_rate);
    return ok ? 0 : -3;
}

extern "C" void jarvis_tts_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_tts_mutex);
    g_tts_ctx = TtsContext{};
}
