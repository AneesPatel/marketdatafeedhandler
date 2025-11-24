#include <gtest/gtest.h>
#include "lock_free_queue.hpp"
#include <thread>
#include <vector>

TEST(SPSCQueueTest, BasicPushPop) {
    SPSCQueue<int> queue(16);
    
    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.try_push(42));
    EXPECT_FALSE(queue.empty());
    
    auto value = queue.try_pop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
    EXPECT_TRUE(queue.empty());
}

TEST(SPSCQueueTest, MultipleElements) {
    SPSCQueue<int> queue(16);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.try_push(i));
    }
    
    for (int i = 0; i < 10; ++i) {
        auto value = queue.try_pop();
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, i);
    }
}

TEST(SPSCQueueTest, FullQueue) {
    SPSCQueue<int> queue(4);
    
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    
    EXPECT_FALSE(queue.try_push(4));
}

TEST(SPSCQueueTest, EmptyQueue) {
    SPSCQueue<int> queue(16);
    
    auto value = queue.try_pop();
    EXPECT_FALSE(value.has_value());
}

TEST(SPSCQueueTest, ConcurrentOperations) {
    const size_t iterations = 100000;
    SPSCQueue<size_t> queue(1024);
    
    std::thread producer([&queue, iterations]() {
        for (size_t i = 0; i < iterations; ++i) {
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });
    
    std::thread consumer([&queue, iterations]() {
        for (size_t i = 0; i < iterations; ++i) {
            std::optional<size_t> value;
            while (!(value = queue.try_pop())) {
                std::this_thread::yield();
            }
            EXPECT_EQ(*value, i);
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_TRUE(queue.empty());
}

TEST(MPSCQueueTest, BasicPushPop) {
    MPSCQueue<int> queue;
    
    queue.push(42);
    
    auto value = queue.try_pop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST(MPSCQueueTest, MultipleProducers) {
    const size_t num_producers = 4;
    const size_t items_per_producer = 10000;
    MPSCQueue<size_t> queue;
    
    std::vector<std::thread> producers;
    for (size_t p = 0; p < num_producers; ++p) {
        producers.emplace_back([&queue, p, items_per_producer]() {
            for (size_t i = 0; i < items_per_producer; ++i) {
                queue.push(p * items_per_producer + i);
            }
        });
    }
    
    for (auto& t : producers) {
        t.join();
    }
    
    std::vector<size_t> received;
    std::optional<size_t> value;
    while ((value = queue.try_pop())) {
        received.push_back(*value);
    }
    
    EXPECT_EQ(received.size(), num_producers * items_per_producer);
}

TEST(SPSCQueueTest, MoveSemantics) {
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
    };
    
    SPSCQueue<MoveOnly> queue(16);
    
    queue.try_push(MoveOnly(42));
    
    auto value = queue.try_pop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->value, 42);
}
