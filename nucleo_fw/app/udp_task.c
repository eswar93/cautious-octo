/* FreeRTOS task: UDP socket <-> app logic. Uses the lwIP SOCKET API (not raw API), so no
 * tcpip-thread locking surprises. Create it from CubeMX's user-code section, after MX_LWIP_Init():
 *     xTaskCreate(udp_task, "udp", 2048, NULL, tskIDLE_PRIORITY + 3, NULL);   // stack is in WORDS
 *
 * Requires in LWIP/Target/lwipopts.h (inside a USER CODE section):
 *     #define LWIP_SO_RCVTIMEO 1
 */
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "stm32f7xx_hal.h"
#include "app.h"

#if !LWIP_SO_RCVTIMEO
#error "Add '#define LWIP_SO_RCVTIMEO 1' to lwipopts.h (USER CODE BEGIN 1 section): without it recvfrom() never times out and LINK LOST can't be detected."
#endif

#define LISTEN_PORT 14550

void udp_task(void *arg)
{
    (void)arg;
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
    if (s < 0 || lwip_bind(s, (struct sockaddr *)&me, sizeof me) < 0) {
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));      /* socket setup failed: park here (add a log/LED) */
    }
#if LWIP_SO_RCVTIMEO == 1 && LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int to = 100;                                      /* some lwIP builds take milliseconds as int */
#else
    struct timeval to = { .tv_sec = 0, .tv_usec = 100000 };   /* 100 ms poll tick */
#endif
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to);

    for (;;) {
        plen = sizeof peer;
        int n = lwip_recvfrom(s, rx, sizeof rx, 0, (struct sockaddr *)&peer, &plen);
        uint32_t now = HAL_GetTick();
        if (n > 0) { have_peer = true; app_on_bytes(&app, rx, (uint16_t)n, now); }

        uint16_t tl;
        if (app_poll(&app, now, tx, &tl) && have_peer)
            lwip_sendto(s, tx, tl, 0, (struct sockaddr *)&peer, sizeof peer);
    }
}
