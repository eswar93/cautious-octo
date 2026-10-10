#include "link_monitor.h"
#include <string.h>

void lm_init(lm_t *m, uint32_t timeout_ms)
{
    memset(m, 0, sizeof *m);
    m->timeout_ms = timeout_ms;
}

void lm_on_packet(lm_t *m, uint32_t now_ms, uint8_t seq)
{
    if (m->have_seq) {
        uint8_t gap = (uint8_t)(seq - m->last_seq - 1u);  /* wraps correctly at 255 */
        m->lost_count += gap;
    }
    m->last_seq = seq;
    m->have_seq = true;
    m->last_rx_ms = now_ms;
    m->seen_any = true;
    m->rx_count++;
}

bool lm_is_up(const lm_t *m, uint32_t now_ms)
{
    return m->seen_any && (uint32_t)(now_ms - m->last_rx_ms) < m->timeout_ms;
}

float lm_loss_ratio(const lm_t *m)
{
    uint32_t total = m->rx_count + m->lost_count;
    return total ? (float)m->lost_count / (float)total : 0.0f;
}
