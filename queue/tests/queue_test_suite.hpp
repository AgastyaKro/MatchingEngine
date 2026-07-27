#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace queue_tests {

inline void require(bool cond, std::string_view msg) {
    if (!cond) throw std::runtime_error(std::string(msg));
}

#define RUN(expr) \
    do { std::cerr << "  " #expr "\n"; expr; } while (0)

template <typename Queue>
void testEmpty() {
    Queue q;
    require(!q.pop().has_value(), "empty queue returned a value");
}

template <typename Queue>
void testSinglePushPop() {
    Queue q;
    require(q.push(42), "push failed");
    auto v = q.pop();
    require(v && *v == 42, "pop returned wrong value");
    require(!q.pop().has_value(), "queue should be empty");
}

template <typename Queue>
void testFIFO() {
    Queue q;
    require(q.push(10) && q.push(20) && q.push(30), "push failed");
    auto a = q.pop(), b = q.pop(), c = q.pop();
    require(a && *a == 10, "FIFO wrong at 10");
    require(b && *b == 20, "FIFO wrong at 20");
    require(c && *c == 30, "FIFO wrong at 30");
}

template <typename Queue, std::int64_t Capacity>
void testFull() {
    Queue q;
    for (std::int64_t i = 0; i < Capacity; ++i)
        require(q.push(static_cast<std::uint64_t>(i)),
                "full too early at index " + std::to_string(i));
    require(!q.push(999), "accepted a value while full");
    for (std::int64_t i = 0; i < Capacity; ++i) {
        auto v = q.pop();
        require(v && *v == static_cast<std::uint64_t>(i),
                "wrong value from full queue at index " + std::to_string(i));
    }
    require(!q.pop().has_value(), "should be empty");
}

// One producer, N consumers. Every item consumed exactly once.
template <typename Queue>
void testExactlyOnce(std::int64_t consumerCount, std::int64_t itemCount) {
    Queue q;
    std::atomic<std::int64_t> consumed{0};
    std::atomic<bool> producerDone{false};
    std::vector<std::vector<std::uint64_t>> results(consumerCount);
    std::vector<std::thread> consumers;

    for (std::int64_t i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&, i] {
            while (true) {
                if (auto v = q.pop()) {
                    results[i].push_back(*v);
                    consumed.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
                if (producerDone.load(std::memory_order_acquire) &&
                    consumed.load(std::memory_order_relaxed) >= itemCount)
                    break;
                std::this_thread::yield();
            }
        });
    }

    for (std::int64_t i = 0; i < itemCount; ++i)
        while (!q.push(static_cast<std::uint64_t>(i))) std::this_thread::yield();
    producerDone.store(true, std::memory_order_release);

    for (auto& c : consumers) c.join();

    std::vector<std::uint64_t> all;
    for (auto& r : results) all.insert(all.end(), r.begin(), r.end());
    require(static_cast<std::int64_t>(all.size()) == itemCount,
            "items lost or duplicated: got " + std::to_string(all.size()) +
            " expected " + std::to_string(itemCount));
    std::sort(all.begin(), all.end());
    for (std::int64_t i = 0; i < itemCount; ++i)
        require(all[i] == static_cast<std::uint64_t>(i),
                "missing or duplicate item at index " + std::to_string(i) +
                ": got " + std::to_string(all[i]));
}

template <typename Queue, std::int64_t Capacity>
void runCorrectnessTests() {
    RUN((testEmpty<Queue>()));
    RUN((testSinglePushPop<Queue>()));
    RUN((testFIFO<Queue>()));
    RUN((testFull<Queue, Capacity>()));
    RUN((testExactlyOnce<Queue>(1, 500)));
    RUN((testExactlyOnce<Queue>(2, 500)));
    RUN((testExactlyOnce<Queue>(3, 500)));
}

} // namespace queue_tests