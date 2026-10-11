// Tiny FIFO ring buffer with static storage (no heap). Not thread-safe: use from one task.
#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace tp {

template <typename T, std::size_t N>
class FixedQueue {
    static_assert(N > 0);
    static_assert(std::is_trivially_copyable_v<T>, "keep it simple: plain data only");

public:
    // Returns false (and drops the new item) when full.
    bool push(const T& item) noexcept {
        if (count_ == N) return false;
        buf_[(head_ + count_) % N] = item;
        ++count_;
        return true;
    }

    [[nodiscard]] std::optional<T> pop() noexcept {
        if (count_ == 0) return std::nullopt;
        const T item = buf_[head_];
        head_ = (head_ + 1) % N;
        --count_;
        return item;
    }

    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    std::array<T, N> buf_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

}  // namespace tp
