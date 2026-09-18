#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <vector>

#include "tp/thread_pool.hpp"

TEST(ThreadPool, ReturnsResultViaFuture) {
    tp::ThreadPool pool(2);
    auto f = pool.submit([](int a, int b) { return a + b; }, 2, 3);
    EXPECT_EQ(f.get(), 5);
}

TEST(ThreadPool, RunsAllTasksBeforeDestruction) {
    std::atomic<int> counter{0};
    {
        tp::ThreadPool pool(4);
        for (int i = 0; i < 1000; ++i)
            pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }  // destructor drains the queue and joins the workers
    EXPECT_EQ(counter.load(), 1000);
}

TEST(ThreadPool, PropagatesExceptions) {
    tp::ThreadPool pool(1);
    auto f = pool.submit([]() -> int { throw std::runtime_error("boom"); });
    EXPECT_THROW(f.get(), std::runtime_error);
}

TEST(ThreadPool, ManyResults) {
    tp::ThreadPool pool(4);
    std::vector<std::future<int>> futs;
    for (int i = 0; i < 100; ++i)
        futs.push_back(pool.submit([i] { return i * i; }));
    long long sum = 0;
    for (auto& f : futs) sum += f.get();
    EXPECT_EQ(sum, 328350);  // sum of squares 0..99
}
