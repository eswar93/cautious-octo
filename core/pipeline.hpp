// Transport-independent heart of the base station: packets and samples in, alert events out.
// No MAVLink, no HAL, no lwIP -> fully testable on the PC.
#pragma once
#include <cstdint>
#include <optional>
#include "fixed_queue.hpp"
#include "link_monitor.hpp"
#include "predictor.hpp"
#include "window.hpp"

namespace tp {

enum class AlertKind : std::uint8_t { DropPredicted, DropCleared, LinkLost, LinkUp };

struct AlertEvent {
    AlertKind kind;
    float probability;  // last model output (meaningful for the Drop* kinds)
};

struct PipelineConfig {
    std::uint32_t link_timeout_ms = 3000;  // 3 missed 1 Hz heartbeats
    std::uint8_t debounce = 3;
};

class Pipeline {
public:
    explicit Pipeline(const Model& model, PipelineConfig cfg = {}) noexcept
        : model_{&model}, link_{cfg.link_timeout_ms}, debounce_{cfg.debounce} {}

    // Call for EVERY valid packet from the remote (heartbeat or data).
    void on_packet(std::uint32_t now_ms, std::uint8_t seq) noexcept {
        if (link_.seen_any() && !link_.is_up(now_ms)) window_.clear();  // gap: window is stale
        link_.on_packet(now_ms, seq);
    }

    // Call per sensor reading; sensor_ms is the remote's timestamp.
    void on_sample(std::uint32_t sensor_ms, float value) noexcept {
        window_.push(sensor_ms, value);
        const auto f = window_.features(model_->window_n);
        if (!f) return;  // still filling the window
        last_p_ = predict_logistic(*model_, *f);
        if (debounce_.update(last_p_ > model_->threshold)) {
            emit(debounce_.active() ? AlertKind::DropPredicted : AlertKind::DropCleared);
        }
    }

    // Call regularly (>= 10 Hz) and drain it: `while (auto e = p.poll(now)) { ... }`
    [[nodiscard]] std::optional<AlertEvent> poll(std::uint32_t now_ms) noexcept {
        const bool up = link_.is_up(now_ms);
        if (link_.seen_any() && up != link_was_up_) {
            link_was_up_ = up;
            if (!up) debounce_.reset();
            emit(up ? AlertKind::LinkUp : AlertKind::LinkLost);
        }
        return events_.pop();
    }

    [[nodiscard]] const LinkMonitor& link() const noexcept { return link_; }
    [[nodiscard]] float last_probability() const noexcept { return last_p_; }

private:
    void emit(AlertKind kind) noexcept { (void)events_.push(AlertEvent{kind, last_p_}); }

    const Model* model_;
    Window window_;
    LinkMonitor link_;
    Debouncer debounce_;
    FixedQueue<AlertEvent, 4> events_;
    float last_p_ = 0.0f;
    bool link_was_up_ = false;
};

}  // namespace tp
