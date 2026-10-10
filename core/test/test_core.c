#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "../tw.h"
#include "../link_monitor.h"
#include "../predictor.h"
#include "../model_weights.h"
#include "golden.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void test_features_linear(void)
{
    tw_t w; tw_init(&w);
    for (int i = 0; i < 40; i++) tw_push(&w, 1000u + i * 100u, 50.0f - 2.0f * (i * 0.1f)); /* slope -2/s */
    float f[TW_NFEAT];
    CHECK(tw_features(&w, 32, f));
    CHECK(fabsf(f[2] + 2.0f) < 1e-3f);
    CHECK(fabsf(f[4] - (50.0f - 2.0f * 3.9f)) < 1e-3f);
    CHECK(!tw_features(&w, 64, f));              /* not enough samples */
}

static void test_timestamp_wrap(void)
{
    tw_t w; tw_init(&w);
    for (int i = 0; i < 10; i++) tw_push(&w, 0xFFFFFFF0u + (uint32_t)(i * 100), (float)i);
    float f[TW_NFEAT];
    CHECK(tw_features(&w, 10, f));
    CHECK(fabsf(f[2] - 10.0f) < 1e-3f);          /* 1 unit / 100 ms across u32 wrap */
}

static void test_link_monitor(void)
{
    lm_t m; lm_init(&m, 3000);
    CHECK(!lm_is_up(&m, 0));
    lm_on_packet(&m, 1000, 254); lm_on_packet(&m, 1100, 255); lm_on_packet(&m, 1200, 2); /* lost 0,1 */
    CHECK(m.lost_count == 2);
    CHECK(lm_is_up(&m, 3000));
    CHECK(!lm_is_up(&m, 4300));
}

static void test_alert_debounce(void)
{
    alert_t a; alert_init(&a, 3);
    CHECK(!alert_update(&a, true)); CHECK(!alert_update(&a, true));
    CHECK(alert_update(&a, true) && a.active);
    CHECK(!alert_update(&a, false));
    CHECK(!alert_update(&a, true));              /* streak broken */
    CHECK(a.active);
}

static void test_rule(void)
{
    float f[TW_NFEAT] = {0, 0, -4.0f, 0, 95.0f};
    CHECK(predict_rule(f, 2.0f, 90.0f));         /* 95 - 8 = 87 < 90 */
    f[2] = -1.0f;
    CHECK(!predict_rule(f, 2.0f, 90.0f));
}

static void test_golden_matches_python(void)
{
    tw_t w; tw_init(&w);
    for (int i = 0; i < GOLDEN_N; i++) tw_push(&w, golden_t_ms[i], golden_v[i]);
    float f[TW_NFEAT];
    CHECK(tw_features(&w, GOLDEN_N, f));
    float p = predict_logistic(&g_model, f);
    printf("golden: C=%.6f  python=%.6f\n", p, golden_p);
    CHECK(fabsf(p - golden_p) < 1e-3f);
}

int main(void)
{
    test_features_linear(); test_timestamp_wrap(); test_link_monitor();
    test_alert_debounce(); test_rule(); test_golden_matches_python();
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails ? 1 : 0;
}
