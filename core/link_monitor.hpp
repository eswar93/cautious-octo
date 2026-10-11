// Tells "sensor is dropping" apart from "link is dead". Feed it every received packet.
#pragma once
#include <cstdint>

namespace tp {

class LinkMonitor {
public:
    explicit constexpr LinkMonitor(std::uint32_t timeout_ms) noexcept : timeout_ms_{timeout_ms} {}

    void on_packet(std::uint32_t now_ms, std::uint8_t seq) noexcept {
        if (have_seq_) {
            // wraps correctly at 255 -> 0
            lost_ += static_cast<std::uint8_t>(seq - last_seq_ - 1u);
        }
        last_seq_ = seq;
        have_seq_ = true;
        last_rx_ms_ = now_ms;
        seen_any_ = true;
        ++received_;
    }

    [[nodiscard]] bool seen_any() const noexcept { return seen_any_; }
    [[nodiscard]] bool is_up(std::uint32_t now_ms) const noexcept {
        return seen_any_ && static_cast<std::uint32_t>(now_ms - last_rx_ms_) < timeout_ms_;
    }
    [[nodiscard]] std::uint32_t received() const noexcept { return received_; }
    [[nodiscard]] std::uint32_t lost() const noexcept { return lost_; }
    [[nodiscard]] float loss_ratio() const noexcept {
        const std::uint32_t total = received_ + lost_;
        return total ? static_cast<float>(lost_) / static_cast<float>(total) : 0.0f;
    }

private:
    std::uint32_t timeout_ms_;
    std::uint32_t last_rx_ms_ = 0;
    std::uint32_t received_ = 0;
    std::uint32_t lost_ = 0;  // inferred from MAVLink sequence gaps
    std::uint8_t last_seq_ = 0;
    bool have_seq_ = false;
    bool seen_any_ = false;
};

}  // namespace tp
