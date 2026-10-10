/* FreeRTOS task: UDP socket <-> app logic. Uses the lwIP SOCKET API (not raw API), so no
 * tcpip-thread locking surprises. Create it from CubeMX's user-code section, after MX_LWIP_Init():
 *     xTaskCreate(udp_task, "udp", 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
 */
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/sockets.h"
#include "stm32f7xx_hal.h"
#include "app.h"

#define LISTEN_PORT 14550

void udp_task(void *arg)
{
    static app_t app;
    uint8_t rx[300], tx[MAVLINK_MAX_PACKET_LEN];
    struct sockaddr_in me = { 0 }, peer = { 0 };
    socklen_t plen = sizeof peer;
    bool have_peer = false;

    app_init(&app);
    int s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    me.sin_family = AF_INET;
    me.sin_port = htons(LISTEN_PORT);
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    lwip_bind(s, (struct sockaddr *)&me, sizeof me);
    struct timeval to = { .tv_sec = 0, .tv_usec = 100000 };   /* 100 ms poll tick */
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to);

    for (;;) {
        int n = lwip_recvfrom(s, rx, sizeof rx, 0, (struct sockaddr *)&peer, &plen);
        uint32_t now = HAL_GetTick();
        if (n > 0) { have_peer = true; app_on_bytes(&app, rx, (uint16_t)n, now); }

        uint16_t tl;
        if (app_poll(&app, now, tx, &tl) && have_peer)
            lwip_sendto(s, tx, tl, 0, (struct sockaddr *)&peer, sizeof peer);
    }
}
