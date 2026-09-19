#pragma once
#include <atomic>
#include <cstddef>
#include <utility>
#include <vector>

namespace tp {

// Lock-free single-producer / single-consumer ring buffer.
// Exactly ONE thread may call try_push and exactly ONE thread may call try_pop.
// T must be default-constructible and movable/assignable.
template <class T>
class SpscQueue {
public:
    explicit SpscQueue(std::size_t capacity)
        : capacity_(round_up_pow2(capacity)),
          mask_(capacity_ - 1),
          buf_(capacity_) {}

    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    std::size_t capacity() const { return capacity_; }

    // Producer thread only. Returns false (and leaves 'value' untouched) if full.
    template <class U>
    bool try_push(U&& value) {
        const std::size_t t = tail_.load(std::memory_order_relaxed);  // only we write tail_
        const std::size_t h = head_.load(std::memory_order_acquire);  // see consumer's progress
        if (t - h == capacity_) return false;                         // full
        buf_[t & mask_] = std::forward<U>(value);
        tail_.store(t + 1, std::memory_order_release);                // publish the element
        return true;
    }

    // Consumer thread only. Returns false if empty.
    bool try_pop(T& out) {
        const std::size_t h = head_.load(std::memory_order_relaxed);  // only we write head_
        const std::size_t t = tail_.load(std::memory_order_acquire);  // see producer's data
        if (h == t) return false;                                     // empty
        out = std::move(buf_[h & mask_]);
        head_.store(h + 1, std::memory_order_release);                // free the slot
        return true;
    }

private:
    static std::size_t round_up_pow2(std::size_t n) {
        std::size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    static constexpr std::size_t kCacheLine = 64;

    const std::size_t capacity_;  // power of two
    const std::size_t mask_;      // capacity_ - 1, replaces the slow % operator
    std::vector<T> buf_;

    // Separate cache lines so producer and consumer don't fight over one (false sharing).
    alignas(kCacheLine) std::atomic<std::size_t> head_{0};  // next slot to read
    alignas(kCacheLine) std::atomic<std::size_t> tail_{0};  // next slot to write
};

}  // namespace tp
