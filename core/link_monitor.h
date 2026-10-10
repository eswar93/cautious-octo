/* Distinguishes "sensor is dropping" from "link is dead". Feed it every received MAVLink packet. */
#ifndef LINK_MONITOR_H
#define LINK_MONITOR_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t last_rx_ms;
    uint32_t timeout_ms;
    uint32_t rx_count;
    uint32_t lost_count;   /* inferred from MAVLink sequence gaps */
    uint8_t  last_seq;
    bool     have_seq;
    bool     seen_any;
} lm_t;

void lm_init(lm_t *m, uint32_t timeout_ms);
void lm_on_packet(lm_t *m, uint32_t now_ms, uint8_t seq);
bool lm_is_up(const lm_t *m, uint32_t now_ms);
float lm_loss_ratio(const lm_t *m);

#endif
