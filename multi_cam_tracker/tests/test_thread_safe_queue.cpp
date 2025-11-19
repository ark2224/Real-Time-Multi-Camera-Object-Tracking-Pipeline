#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "ThreadSafeQueue.hpp"
#include <thread>
#include <chrono>
#include <atomic>


// Test 1 - Basic push/pop; single thread
TEST_CASE("ThreadSafeQueue basic push/pop in single thread", "[ThreadSafeQueue]") {
    ThreadSafeQueue<int> q;

    q.push(1);
    q.push(2);
    q.push(3);

    auto v1 = q.pop();
    REQUIRE(v1.has_value());
    REQUIRE(*v1 == 1);

    auto v2 = q.pop();
    REQUIRE(v2.has_value());
    REQUIRE(*v2 == 2);

    auto v3 = q.pop();
    REQUIRE(v3.has_value());
    REQUIRE(*v3 == 3);
}

// Test 2 - pop() blocks empty queue (until a push)
TEST_CASE("ThreadSafeQueue pop blocks until push", "[ThreadSafeQueue]") {
    ThreadSafeQueue<int> q;

    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    int result = 0;

    std::thread consumer([&]() {
        started = true;
        auto v = q.pop();
        if (v.has_value()) {
            result = *v;
        }
        finished = true;
    });

    // Wait until consumer thread calls pop()
    while (!started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Sleep a bit to demonstrate that pop is indeed blocking
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(!finished);

    // Now push a value, thereby unblocking pop()
    q.push(42);

    // Let consumer process
    consumer.join();

    REQUIRE(finished);
    REQUIRE(result == 42);
}


// Test 3 - stop() unblocks waiting during pop()
TEST_CASE("ThreadSafeQueue stop unblocks waiting pop", "[ThreadSafeQueue]") {
    ThreadSafeQueue<int> q;

    std::atomic<bool> woke(false);
    std::optional<int> result;

    std::thread consumer([&]() {
        result = q.pop();
        woke = true;
    });

    // Give thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // unblock consumer with stop
    q.stop();

    consumer.join();

    REQUIRE(woke);
    REQUIRE_FALSE(result.has_value());
}


// Test 4 - stop() on an empty queue makes pop return the null option
TEST_CASE("ThreadSafeQueue pop after stop returns nulllopt on empty queue", "[ThreadSafeQueue]") {
    ThreadSafeQueue<int> q;

    q.stop();
    auto v = q.pop();
    REQUIRE_FALSE(v.has_value());
}


// Test 5 - multiple producers/consumers config
TEST_CASE("ThreadSafeQueue supporting multiple consumers and producers", "[ThreadSafeQueue]") {
    ThreadSafeQueue<int> q;

    const int numProducers = 4;
    const int numConsumers = 4;
    const int itemsPerProducer = 25;
    const int totalItems = numProducers * itemsPerProducer;

    std::atomic<int> consumedCount{0};

    // Producers pushing (somewhat) unique values
    std::vector<std::thread> producers;
    for (int p = 0; p < numProducers; ++p) {
        producers.emplace_back([&, p]() {
            for (int i = 0; i < itemsPerProducer; ++i) {
                q.push(p * 1000 + i);
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    consumers.reserve(numConsumers);

    for (int c = 0; c < numConsumers; ++c) {
        consumers.emplace_back([&]() {
            while (true) {
                auto v = q.pop();
                if (!v.has_value()) {
                    break;
                }
                consumedCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Wait for all producers then stop queue
    for (auto& t : producers) {
        t.join();
    }

    q.stop();

    for (auto& t : consumers) {
        t.join();
    }

    REQUIRE(consumedCount == totalItems);
}