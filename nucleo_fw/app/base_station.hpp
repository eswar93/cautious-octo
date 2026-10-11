// MAVLink adapter around tp::Pipeline: raw datagram bytes in, serialized STATUSTEXT alerts out.
// No HAL / lwIP here, so it stays testable on a PC if you add a MAVLink-enabled test later.
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "pipeline.hpp"
#include "model_weights.hpp"
#include "mavlink.h"

namespace app {

static_assert(tp::kModel.window_n <= tp::Window::capacity(),
              "model window is larger than the sample window: raise Window capacity");

class BaseStation {
public:
    static constexpr std::uint8_t kSysId = 1;

    BaseStation() noexcept : pipeline_{tp::kModel} {}

    // Feed whatever recvfrom() returned (may contain 0..n MAVLink frames, or partial ones).
    void on_bytes(std::span<const std::uint8_t> data, std::uint32_t now_ms) noexcept;

    // Serialises the next pending alert into `out` (>= MAVLINK_MAX_PACKET_LEN bytes) and returns
    // its length, or nullopt if there is nothing to send. Call in a loop until it returns nullopt.
    [[nodiscard]] std::optional<std::size_t> poll(std::uint32_t now_ms,
                                                  std::span<std::uint8_t> out) noexcept;

private:
    tp::Pipeline pipeline_;
    mavlink_status_t parse_status_{};
};

}  // namespace app
