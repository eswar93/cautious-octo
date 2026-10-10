#include "tw.h"
#include <math.h>
#include <string.h>

void tw_init(tw_t *w) { memset(w, 0, sizeof *w); }

void tw_push(tw_t *w, uint32_t t_ms, float v)
{
    w->v[w->head]    = v;
    w->t_ms[w->head] = t_ms;
    w->head = (uint16_t)((w->head + 1u) % TW_CAPACITY);
    if (w->count < TW_CAPACITY) w->count++;
}

uint16_t tw_count(const tw_t *w) { return w->count; }

static uint16_t idx_oldest_of_n(const tw_t *w, uint16_t n)
{
    return (uint16_t)((w->head + TW_CAPACITY - n) % TW_CAPACITY);
}

bool tw_features(const tw_t *w, uint16_t n, float out[TW_NFEAT])
{
    if (n < 2 || n > TW_CAPACITY || w->count < n) return false;

    uint16_t i0 = idx_oldest_of_n(w, n);
    uint32_t t0 = w->t_ms[i0];

    double sv = 0, st = 0;
    for (uint16_t k = 0; k < n; k++) {
        uint16_t i = (uint16_t)((i0 + k) % TW_CAPACITY);
        sv += w->v[i];
        st += (double)(uint32_t)(w->t_ms[i] - t0) * 1e-3;
    }
    double mean_v = sv / n, mean_t = st / n;

    double var = 0, sxy = 0, sxx = 0;
    for (uint16_t k = 0; k < n; k++) {
        uint16_t i = (uint16_t)((i0 + k) % TW_CAPACITY);
        double dv = w->v[i] - mean_v;
        double dt = (double)(uint32_t)(w->t_ms[i] - t0) * 1e-3 - mean_t;
        var += dv * dv;
        sxy += dt * dv;
        sxx += dt * dt;
    }
    uint16_t il = (uint16_t)((i0 + n - 1) % TW_CAPACITY);

    out[0] = (float)mean_v;
    out[1] = (float)sqrt(var / n);
    out[2] = (sxx > 0.0) ? (float)(sxy / sxx) : 0.0f;
    out[3] = w->v[il] - w->v[i0];
    out[4] = w->v[il];
    return true;
}
