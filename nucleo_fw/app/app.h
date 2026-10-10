/* Base-station application logic: MAVLink bytes in -> prediction/alerts out.
 * No HAL or lwIP here, so it can be unit-tested on the PC. */
#ifndef APP_H
#define APP_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tw.h"
#include "link_monitor.h"
#include "predictor.h"
#include "mavlink.h"

typedef struct {
    tw_t     win;
    lm_t     link;
    alert_t  alert;
    float    last_p;
    bool     link_was_up;
    mavlink_status_t parse_status;
    char     pending[48];     /* single-slot outbox for the next STATUSTEXT */
    uint8_t  pending_sev;
    bool     has_pending;
} app_t;

void app_init(app_t *a);
void app_on_bytes(app_t *a, const uint8_t *data, uint16_t len, uint32_t now_ms);
/* Call at >= 10 Hz. Returns true and fills out/out_len when a STATUSTEXT should be sent. */
bool app_poll(app_t *a, uint32_t now_ms, uint8_t *out, uint16_t *out_len);

#endif
