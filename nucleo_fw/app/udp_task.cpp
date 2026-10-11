// FreeRTOS task: UDP socket <-> BaseStation. Uses the lwIP SOCKET API (not the raw API), so there
// are no tcpip-thread locking surprises. Start it from CubeMX's freertos.c (C file), after
// MX_LWIP_Init():
//     xTaskCreate(udp_task, "udp", 2048, NULL, tskIDLE_PRIORITY + 3, NULL);   // stack in WORDS
//
// Requires in LWIP/Target/lwipopts.h (USER CODE section):  #define LWIP_SO_RCVTIMEO 1
#include <array>
#include <cstddef>
#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "stm32f7xx_hal.h"

#include "base_station.hpp"

#if !LWIP_SO_RCVTIMEO
#error "Add '#define LWIP_SO_RCVTIMEO 1' to lwipopts.h (USER CODE BEGIN 1): without it recvfrom() never times out and LINK LOST can't be detected."
#endif

namespace {

constexpr std::uint16_t kListenPort = 14550;

// Namespace-scope on purpose: avoids function-local-static guards (we build with
// -fno-threadsafe-statics). Constructed by __libc_init_array() before main().
app::BaseStation g_station;

}  // namespace

extern "C" void udp_task(void* /*arg*/) {
    std::array<std::uint8_t, 300> rx{};
    std::array<std::uint8_t, MAVLINK_MAX_PACKET_LEN> tx{};
    sockaddr_in me{};
    sockaddr_in peer{};
    bool have_peer = false;

    const int sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    me.sin_family = AF_INET;
    me.sin_port = lwip_htons(kListenPort);
    me.sin_addr.s_addr = lwip_htonl(INADDR_ANY);
    if (sock < 0 || lwip_bind(sock, reinterpret_cast<sockaddr*>(&me), sizeof me) < 0) {
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));   // setup failed: park here (add a log/LED)
    }

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    const int timeout = 100;                        // some lwIP builds take milliseconds as int
#else
    const timeval timeout{0, 100000};               // 100 ms poll tick
#endif
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    for (;;) {
        socklen_t peer_len = sizeof peer;
        const int n = lwip_recvfrom(sock, rx.data(), rx.size(), 0,
                                    reinterpret_cast<sockaddr*>(&peer), &peer_len);
        const std::uint32_t now = HAL_GetTick();
        if (n > 0) {
            have_peer = true;
            g_station.on_bytes({rx.data(), static_cast<std::size_t>(n)}, now);
        }
        while (const auto len = g_station.poll(now, tx)) {
            if (have_peer) {
                lwip_sendto(sock, tx.data(), *len, 0,
                            reinterpret_cast<const sockaddr*>(&peer), sizeof peer);
            }
        }
    }
}
