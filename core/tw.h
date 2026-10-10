/* Sliding window of timestamped samples + feature extraction.
 * Portable C99: no HAL, no allocation. Same code runs on the Nucleo and on the PC (tests). */
#ifndef TW_H
#define TW_H
#include <stdint.h>
#include <stdbool.h>

#define TW_CAPACITY 64
#define TW_NFEAT    5   /* keep in sync with ml/features.py */

typedef struct {
    float    v[TW_CAPACITY];
    uint32_t t_ms[TW_CAPACITY];   /* SENSOR timestamps (ESP32 time), not arrival time */
    uint16_t head;                /* next write index */
    uint16_t count;
} tw_t;

void     tw_init(tw_t *w);
void     tw_push(tw_t *w, uint32_t t_ms, float v);
uint16_t tw_count(const tw_t *w);

/* Features over the newest n samples (n <= TW_CAPACITY, n >= 2):
 *  [0] mean   [1] std   [2] slope (units/s, least squares)
 *  [3] last - first      [4] last
 * Returns false if fewer than n samples are available. */
bool tw_features(const tw_t *w, uint16_t n, float out[TW_NFEAT]);

#endif
