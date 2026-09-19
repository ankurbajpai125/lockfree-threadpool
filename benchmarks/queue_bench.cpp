#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include "tp/mutex_queue.hpp"
#include "tp/spsc_queue.hpp"

constexpr std::uint64_t kItems = 1'000'000;
constexpr std::size_t kCapacity = 1024;
constexpr int kRuns = 5;

// One producer thread pushes 0..kItems-1, the main thread consumes them.
template <class Q>
double run_once() {
    Q q(kCapacity);
    std::uint64_t sum = 0;

    const auto start = std::chrono::steady_clock::now();
    std::thread producer([&] {
        for (std::uint64_t i = 0; i < kItems; ++i)
            while (!q.try_push(i)) {}  // spin while full
    });

    std::uint64_t v = 0, got = 0;
    while (got < kItems) {
        if (q.try_pop(v)) {
            sum += v;
            ++got;
        }
    }
    producer.join();
    const auto end = std::chrono::steady_clock::now();

    // Correctness check: every item arrived exactly once.
    if (sum != kItems * (kItems - 1) / 2) {
        std::fprintf(stderr, "checksum mismatch, queue is broken\n");
        std::exit(1);
    }
    return std::chrono::duration<double>(end - start).count();
}

template <class Q>
void bench(const char* name) {
    std::vector<double> times;
    for (int i = 0; i < kRuns; ++i) times.push_back(run_once<Q>());
    std::sort(times.begin(), times.end());
    const double median = times[times.size() / 2];
    std::printf("%-16s median %.3f s  ->  %.2f M items/sec\n", name, median,
                kItems / median / 1e6);
}

int main() {
    std::printf("items/run: %llu, capacity: %zu, runs: %d (median reported)\n",
                static_cast<unsigned long long>(kItems), kCapacity, kRuns);
    bench<tp::MutexQueue<std::uint64_t>>("mutex queue");
    bench<tp::SpscQueue<std::uint64_t>>("lock-free SPSC");
}
