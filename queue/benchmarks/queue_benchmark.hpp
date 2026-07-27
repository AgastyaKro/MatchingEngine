#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

namespace queue_bench {

template <typename Queue>
void runThroughput(std::int64_t consumerCount, std::int64_t itemsCount) {
    Queue q;
    std::atomic<std::int64_t> consumed{0};
    std::vector<std::uint64_t> checksums(
        static_cast<std::size_t>(consumerCount), 0);
    std::vector<std::thread> consumers;

    for (std::int64_t i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&, i]() {
            std::uint64_t local = 0;
            while (consumed.load(std::memory_order_relaxed) < itemsCount) {
                if (auto v = q.pop()) {
                    local += *v;
                    consumed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            checksums[static_cast<std::size_t>(i)] = local;
        });
    }

    const auto start = std::chrono::steady_clock::now();
    for (std::int64_t i = 0; i < itemsCount; ++i)
        while (!q.push(static_cast<std::uint64_t>(i)))
            ;   // queue full — spin until a consumer frees a slot
    for (auto& c : consumers)
        c.join();
    const auto end = std::chrono::steady_clock::now();

    std::uint64_t checksum = 0;
    for (auto c : checksums)
        checksum += c;

    const double secs = std::chrono::duration<double>(end - start).count();

    std::cout << "  consumers=" << consumerCount
              << " items=" << itemsCount
              << " secs=" << secs
              << " items/sec=" << (static_cast<double>(itemsCount) / secs)
              << " checksum=" << checksum << '\n';
}

} // namespace queue_bench