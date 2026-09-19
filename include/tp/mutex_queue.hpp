#pragma once
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace tp {

// Bounded queue protected by a mutex. Same interface as SpscQueue,
// used as the baseline in benchmarks.
template <class T>
class MutexQueue {
public:
    explicit MutexQueue(std::size_t capacity) : cap_(capacity), buf_(capacity) {}

    template <class U>
    bool try_push(U&& value) {
        std::lock_guard<std::mutex> lk(m_);
        if (count_ == cap_) return false;
        buf_[(head_ + count_) % cap_] = std::forward<U>(value);
        ++count_;
        return true;
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lk(m_);
        if (count_ == 0) return false;
        out = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        --count_;
        return true;
    }

private:
    std::mutex m_;
    const std::size_t cap_;
    std::vector<T> buf_;
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

}  // namespace tp
