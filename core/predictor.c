#include "predictor.h"
#include <math.h>

float predict_logistic(const model_t *m, const float feat[TW_NFEAT])
{
    float z = m->b;
    for (int i = 0; i < TW_NFEAT; i++) {
        float s = (m->sd[i] > 1e-9f) ? m->sd[i] : 1.0f;
        z += m->w[i] * (feat[i] - m->mu[i]) / s;
    }
    return 1.0f / (1.0f + expf(-z));
}

bool predict_rule(const float feat[TW_NFEAT], float horizon_s, float floor_value)
{
    float projected = feat[4] + feat[2] * horizon_s;
    return projected < floor_value;
}

void alert_init(alert_t *a, uint8_t need) { a->need = need; a->streak = 0; a->active = false; }

bool alert_update(alert_t *a, bool positive)
{
    if (positive == a->active) { a->streak = 0; return false; }
    if (++a->streak >= a->need) { a->active = positive; a->streak = 0; return true; }
    return false;
}
