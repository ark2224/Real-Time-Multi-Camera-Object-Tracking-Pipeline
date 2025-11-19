#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    void push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(value);
        }
        m_cv.notify_one();
    }

    void push(T&& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
        }
        m_cv.notify_one();
    }

    // Blocks until an item is available or stop() gets called
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] { return !m_queue.empty() || m_stopped; });

        if (m_stopped && m_queue.empty()) {
            return std::nullopt;
        }

        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();
    }


private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopped{false};
};