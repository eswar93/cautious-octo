#ifndef PREDICTOR_H
#define PREDICTOR_H
#include "tw.h"
#include <stdbool.h>

/* Logistic model on standardized features. Weights come from ml/train.py -> model_weights.h */
typedef struct {
    float mu[TW_NFEAT];
    float sd[TW_NFEAT];
    float w[TW_NFEAT];
    float b;
    float threshold;      /* probability above which we call "drop coming" */
    uint16_t window_n;    /* samples per window used in training */
} model_t;

/* Returns P(significant drop within the horizon used for training labels). */
float predict_logistic(const model_t *m, const float feat[TW_NFEAT]);

/* Zero-ML baseline: linear extrapolation. Positive if slope * horizon_s projects below floor. */
bool  predict_rule(const float feat[TW_NFEAT], float horizon_s, float floor_value);

/* Debounce: require `need` consecutive positives to raise, `need` consecutive negatives to clear. */
typedef struct { uint8_t need, streak; bool active; } alert_t;
void alert_init(alert_t *a, uint8_t need);
/* returns true when the active state CHANGED on this update */
bool alert_update(alert_t *a, bool positive);

#endif
