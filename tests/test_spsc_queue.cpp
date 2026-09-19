#include <gtest/gtest.h>

#include <thread>

#include "tp/spsc_queue.hpp"

TEST(SpscQueue, PushPopInOrder) {
    tp::SpscQueue<int> q(4);
    int v = 0;
    EXPECT_FALSE(q.try_pop(v));  // empty
    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 1);
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 2);
    EXPECT_FALSE(q.try_pop(v));
}

TEST(SpscQueue, RejectsPushWhenFull) {
    tp::SpscQueue<int> q(4);
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.try_push(i));
    EXPECT_FALSE(q.try_push(99));
    int v = 0;
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_TRUE(q.try_push(99));  // one slot freed
}

TEST(SpscQueue, CapacityRoundsUpToPowerOfTwo) {
    tp::SpscQueue<int> q(5);
    EXPECT_EQ(q.capacity(), 8u);
}

TEST(SpscQueue, WrapsAroundManyTimes) {
    tp::SpscQueue<int> q(4);
    int v = 0;
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(q.try_push(i));
        EXPECT_TRUE(q.try_pop(v));
        EXPECT_EQ(v, i);
    }
}

TEST(SpscQueue, ProducerConsumerPreservesOrder) {
    constexpr int N = 200000;
    tp::SpscQueue<int> q(1024);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i)
            while (!q.try_push(i)) std::this_thread::yield();
    });

    bool ordered = true;
    int received = 0, v = 0;
    while (received < N) {
        if (q.try_pop(v)) {
            if (v != received) ordered = false;
            ++received;
        } else {
            std::this_thread::yield();
        }
    }
    producer.join();
    EXPECT_TRUE(ordered);
}
