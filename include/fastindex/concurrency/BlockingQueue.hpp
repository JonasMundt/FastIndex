
#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <stop_token>
#include <utility>

namespace fastindex::concurrency {

template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity)
        : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument(
                "Queue capacity must be greater than zero"
            );
        }
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    [[nodiscard]]
    bool push(T value, std::stop_token stop = {}) {
        std::unique_lock lock(mutex_);

        const bool ready = notFull_.wait(
            lock,
            stop,
            [this] {
                return closed_ || queue_.size() < capacity_;
            }
        );

        if (!ready || closed_ || stop.stop_requested()) {
            return false;
        }

        queue_.push(std::move(value));

        lock.unlock();
        notEmpty_.notify_one();

        return true;
    }

    [[nodiscard]]
    std::optional<T> pop(std::stop_token stop = {}) {
        std::unique_lock lock(mutex_);

        const bool ready = notEmpty_.wait(
            lock,
            stop,
            [this] {
                return closed_ || !queue_.empty();
            }
        );

        if (!ready || stop.stop_requested() || queue_.empty()) {
            return std::nullopt;
        }

        std::optional<T> value{
            std::move(queue_.front())
        };

        queue_.pop();

        lock.unlock();
        notFull_.notify_one();

        return value;
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }

        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    const std::size_t capacity_;

    std::queue<T> queue_;
    std::mutex mutex_;

    std::condition_variable_any notEmpty_;
    std::condition_variable_any notFull_;

    bool closed_ = false;
};

} // namespace fastindex::concurrency
