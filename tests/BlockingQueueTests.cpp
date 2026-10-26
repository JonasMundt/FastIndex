#include "fastindex/concurrency/BlockingQueue.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <stop_token>
#include <thread>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

using fastindex::concurrency::BlockingQueue;

TEST(BlockingQueueTest, PreservesFifoOrder) {
    BlockingQueue<int> queue(2);

    ASSERT_TRUE(queue.push(10));
    ASSERT_TRUE(queue.push(20));

    EXPECT_EQ(queue.pop(), std::optional<int>{10});
    EXPECT_EQ(queue.pop(), std::optional<int>{20});
}

TEST(BlockingQueueTest, RejectsZeroCapacity) {
    EXPECT_THROW(
        BlockingQueue<int>(0),
        std::invalid_argument
    );
}

TEST(BlockingQueueTest, ClosePreventsNewItems) {
    BlockingQueue<int> queue(2);

    ASSERT_TRUE(queue.push(42));

    queue.close();

    EXPECT_FALSE(queue.push(100));

    EXPECT_EQ(queue.pop(), std::optional<int>{42});
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(BlockingQueueTest, SupportsCancellation) {
    BlockingQueue<int> queue(2);

    std::optional<int> result;

    std::jthread worker(
        [&](std::stop_token stop) {
            result = queue.pop(stop);
        }
    );

    worker.request_stop();
    worker.join();

    EXPECT_FALSE(result.has_value());
}

TEST(BlockingQueueTest, CancelsBlockedProducer) {
    BlockingQueue<int> queue(1);

    ASSERT_TRUE(queue.push(10));

    bool pushed = true;

    std::jthread worker(
        [&](std::stop_token stop) {
            pushed = queue.push(20, stop);
        }
    );

    worker.request_stop();
    worker.join();

    EXPECT_FALSE(pushed);
    EXPECT_EQ(queue.pop(), std::optional<int>{10});
}


TEST(BlockingQueueTest, HandlesConcurrentProducersAndConsumers) {
    constexpr int producerCount = 4;
    constexpr int consumerCount = 3;
    constexpr int itemsPerProducer = 200;
    constexpr int totalItems = producerCount * itemsPerProducer;

    BlockingQueue<int> queue(8);

    std::vector<int> received;
    received.reserve(totalItems);

    std::mutex receivedMutex;
    std::atomic<int> failedPushes{0};

    std::vector<std::jthread> consumers;

    // Start consumers
    for (int i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&] {
            while (auto value = queue.pop()) {
                std::lock_guard lock(receivedMutex);
                received.push_back(*value);
            }
        });
    }

    std::vector<std::jthread> producers;

    // Start producers
    for (int p = 0; p < producerCount; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 0; i < itemsPerProducer; ++i) {
                const int value = p * itemsPerProducer + i;

                if (!queue.push(value)) {
                    failedPushes.fetch_add(1);
                }
            }
        });
    }

    // Wait for all producers
    for (auto& producer : producers) {
        producer.join();
    }

    // No more items will be added
    queue.close();

    // Wait for all consumers
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(failedPushes.load(), 0);
    ASSERT_EQ(received.size(), totalItems);

    std::sort(received.begin(), received.end());

    // Every value must appear exactly once
    for (int i = 0; i < totalItems; ++i) {
        EXPECT_EQ(received[i], i);
    }
}

TEST(BlockingQueueTest, SupportsMoveOnlyTypes) {
    BlockingQueue<std::unique_ptr<int>> queue(2);

    auto value = std::make_unique<int>(42);

    ASSERT_TRUE(queue.push(std::move(value)));

    // Ownership has moved into the queue
    EXPECT_EQ(value, nullptr);

    auto result = queue.pop();

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result->get(), nullptr);

    EXPECT_EQ(**result, 42);
}

