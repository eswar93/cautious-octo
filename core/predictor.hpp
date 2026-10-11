#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "window.hpp"

namespace tp {

// Logistic model on standardized features. Weights come from ml/train.py -> model_weights.hpp.
struct Model {
    Features mu;
    Features sd;
    Features w;
    float b;
    float threshold;        // probability above which we call "drop coming"
    std::size_t window_n;   // samples per window used in training
};

// P(significant drop within the horizon used for the training labels).
[[nodiscard]] inline float predict_logistic(const Model& m, const Features& f) noexcept {
    float z = m.b;
    for (std::size_t i = 0; i < kFeatureCount; ++i) {
        const float s = m.sd[i] > 1e-9f ? m.sd[i] : 1.0f;
        z += m.w[i] * (f[i] - m.mu[i]) / s;
    }
    return 1.0f / (1.0f + std::exp(-z));
}

// Zero-ML baseline for comparison: linear extrapolation of the recent trend.
[[nodiscard]] constexpr bool predict_rule(const Features& f, float horizon_s,
                                          float floor_value) noexcept {
    return f[feat::last] + f[feat::slope] * horizon_s < floor_value;
}

// Requires `need` consecutive opposite readings before the state flips (kills flapping).
class Debouncer {
public:
    explicit constexpr Debouncer(std::uint8_t need) noexcept : need_{need} {}

    // Returns true when the active state CHANGED on this update.
    [[nodiscard]] bool update(bool positive) noexcept {
        if (positive == active_) { streak_ = 0; return false; }
        if (++streak_ >= need_) { active_ = positive; streak_ = 0; return true; }
        return false;
    }
    void reset() noexcept { active_ = false; streak_ = 0; }
    [[nodiscard]] constexpr bool active() const noexcept { return active_; }

private:
    std::uint8_t need_;
    std::uint8_t streak_ = 0;
    bool active_ = false;
};

}  // namespace tp
