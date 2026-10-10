#include "app.h"
#include <stdio.h>
#include <string.h>
#include "model_weights.h"

#define LINK_TIMEOUT_MS 3000u      /* 3 missed heartbeats */
#define ALERT_DEBOUNCE  3u
#define BASE_SYS_ID     1
#define BASE_COMP_ID    MAV_COMP_ID_ONBOARD_COMPUTER

static void post(app_t *a, uint8_t sev, const char *txt)
{
    strncpy(a->pending, txt, sizeof a->pending - 1);
    a->pending[sizeof a->pending - 1] = 0;
    a->pending_sev = sev;
    a->has_pending = true;
}

void app_init(app_t *a)
{
    memset(a, 0, sizeof *a);
    tw_init(&a->win);
    lm_init(&a->link, LINK_TIMEOUT_MS);
    alert_init(&a->alert, ALERT_DEBOUNCE);
}

static void on_sensor(app_t *a, uint32_t t_ms, float v)
{
    tw_push(&a->win, t_ms, v);
    float f[TW_NFEAT];
    if (!tw_features(&a->win, g_model.window_n, f)) return;   /* still filling window */

#ifdef APP_USE_RULE_BASELINE
    bool positive = predict_rule(f, 2.0f, 85.0f);
    a->last_p = positive ? 1.0f : 0.0f;
#else
    a->last_p = predict_logistic(&g_model, f);
    bool positive = a->last_p > g_model.threshold;
#endif
    if (alert_update(&a->alert, positive)) {
        char txt[48];
        if (a->alert.active) { snprintf(txt, sizeof txt, "DROP PREDICTED p=%.2f", (double)a->last_p); post(a, MAV_SEVERITY_WARNING, txt); }
        else                 { post(a, MAV_SEVERITY_INFO, "drop risk cleared"); }
    }
}

void app_on_bytes(app_t *a, const uint8_t *data, uint16_t len, uint32_t now_ms)
{
    mavlink_message_t msg;
    for (uint16_t i = 0; i < len; i++) {
        if (!mavlink_parse_char(MAVLINK_COMM_0, data[i], &msg, &a->parse_status)) continue;
        if (msg.sysid == BASE_SYS_ID) continue;               /* ignore our own echoes */
        if (!lm_is_up(&a->link, now_ms) && a->link.seen_any) tw_init(&a->win);  /* gap: stale window */
        lm_on_packet(&a->link, now_ms, msg.seq);
        if (msg.msgid == MAVLINK_MSG_ID_NAMED_VALUE_FLOAT) {
            mavlink_named_value_float_t nv;
            mavlink_msg_named_value_float_decode(&msg, &nv);
            if (strncmp(nv.name, "SENSOR", sizeof nv.name) == 0) on_sensor(a, nv.time_boot_ms, nv.value);
        }
    }
}

bool app_poll(app_t *a, uint32_t now_ms, uint8_t *out, uint16_t *out_len)
{
    bool up = lm_is_up(&a->link, now_ms);
    if (a->link.seen_any && up != a->link_was_up) {
        a->link_was_up = up;
        if (!up) { alert_init(&a->alert, ALERT_DEBOUNCE); post(a, MAV_SEVERITY_CRITICAL, "LINK LOST"); }
        else     { post(a, MAV_SEVERITY_INFO, "link restored"); }
    }
    if (!a->has_pending) return false;
    mavlink_message_t m;
    mavlink_msg_statustext_pack(BASE_SYS_ID, BASE_COMP_ID, &m, a->pending_sev, a->pending, 0, 0);
    *out_len = mavlink_msg_to_send_buffer(out, &m);
    a->has_pending = false;
    return true;
}
