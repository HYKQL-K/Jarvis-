#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize wakeword detector. model_path can be NULL if using built-in/stub. (hykql原创)
// sensitivity: 0.0-1.0 (higher = more sensitive). (hykql原创)
int jarvis_wake_init(const char *model_path, float sensitivity);

// Score a PCM16 mono stream. n = sample count (single channel). (hykql原创)
// Writes the most recent wake likelihood into *score if non-null. (hykql原创)
// Returns 1 if the wakeword fired on this call, 0 otherwise; negative on error. (hykql原创)
int jarvis_wake_score(const int16_t *pcm, int n, float *score);

// Reset internal detector state (double-threshold, cooldown, etc.). (hykql原创)
int jarvis_wake_reset(void);

// Free detector resources. (hykql原创)
void jarvis_wake_close(void);

#ifdef __cplusplus
} // extern "C"
#endif
