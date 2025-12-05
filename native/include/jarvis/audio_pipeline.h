#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jarvis_audio_params
{
    int sample_rate_hz;    // e.g., 16000 or 48000. (hykql原创)
    int num_channels;      // 1 (mono) or 2 (stereo). (hykql原创)
    int frame_samples;     // samples per channel per frame (e.g., 160 for 10ms @16k). (hykql原创)
    bool enable_aec;       // Acoustic echo cancellation. (hykql原创)
    bool enable_ns;        // Noise suppression. (hykql原创)
    bool enable_agc;       // Automatic gain control. (hykql原创)
    bool enable_vad;       // Voice activity detection. (hykql原创)
} jarvis_audio_params;

// Initialize the audio pipeline. Returns 0 on success, non-zero on failure. (hykql原创)
int jarvis_audio_init(const jarvis_audio_params *params);

// Process interleaved PCM16 audio in-place. n = samples * channels. (hykql原创)
// Sets *is_speech when VAD is enabled; otherwise leaves it unchanged. (hykql原创)
int jarvis_audio_process(int16_t *pcm, int n, bool *is_speech);

// Tear down and free pipeline resources. (hykql原创)
void jarvis_audio_close(void);

#ifdef __cplusplus
} // extern "C"
#endif
