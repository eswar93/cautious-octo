// Fixed-capacity sliding window of timestamped samples + feature extraction.
// Header-only, no heap, no exceptions: runs unchanged on the Nucleo and on the PC (tests).
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace tp {

inline constexpr std::size_t kFeatureCount = 5;
using Features = std::array<float, kFeatureCount>;

// Indices into Features. MUST match ml/features.py (same order, same definitions).
namespace feat {
inline constexpr std::size_t mean = 0;    // mean of values
inline constexpr std::size_t stddev = 1;  // population std-dev
inline constexpr std::size_t slope = 2;   // least-squares slope, units per second
inline constexpr std::size_t delta = 3;   // last - first
inline constexpr std::size_t last = 4;    // newest value
}  // namespace feat

template <std::size_t Capacity>
class SampleWindow {
    static_assert(Capacity >= 2, "a slope needs at least two samples");

public:
    static constexpr std::size_t capacity() noexcept { return Capacity; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    void clear() noexcept { head_ = 0; count_ = 0; }

    // t_ms is the SENSOR's timestamp (remote clock), never the arrival time.
    void push(std::uint32_t t_ms, float value) noexcept {
        values_[head_] = value;
        times_[head_] = t_ms;
        head_ = (head_ + 1) % Capacity;
        if (count_ < Capacity) ++count_;
    }

    // Features over the newest n samples; nullopt if n is invalid or not enough samples yet.
    [[nodiscard]] std::optional<Features> features(std::size_t n) const noexcept {
        if (n < 2 || n > Capacity || count_ < n) return std::nullopt;

        const std::size_t first = (head_ + Capacity - n) % Capacity;
        const auto index = [&](std::size_t k) { return (first + k) % Capacity; };
        const std::uint32_t t0 = times_[first];
        // unsigned subtraction: stays correct across the uint32 millisecond wrap (~49 days)
        const auto seconds = [&](std::size_t i) {
            return static_cast<double>(static_cast<std::uint32_t>(times_[i] - t0)) * 1e-3;
        };

        const double count = static_cast<double>(n);
        double sum_v = 0.0, sum_t = 0.0;
        for (std::size_t k = 0; k < n; ++k) {
            sum_v += values_[index(k)];
            sum_t += seconds(index(k));
        }
        const double mean_v = sum_v / count, mean_t = sum_t / count;

        double var = 0.0, sxy = 0.0, sxx = 0.0;
        for (std::size_t k = 0; k < n; ++k) {
            const double dv = values_[index(k)] - mean_v;
            const double dt = seconds(index(k)) - mean_t;
            var += dv * dv;
            sxy += dt * dv;
            sxx += dt * dt;
        }

        const std::size_t last_i = index(n - 1);
        Features f{};
        f[feat::mean] = static_cast<float>(mean_v);
        f[feat::stddev] = static_cast<float>(std::sqrt(var / count));
        f[feat::slope] = sxx > 0.0 ? static_cast<float>(sxy / sxx) : 0.0f;
        f[feat::delta] = values_[last_i] - values_[first];
        f[feat::last] = values_[last_i];
        return f;
    }

private:
    std::array<float, Capacity> values_{};
    std::array<std::uint32_t, Capacity> times_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

using Window = SampleWindow<64>;

}  // namespace tp
