#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <x86intrin.h>   
#include <immintrin.h>   
#include <algorithm>

namespace queue_bench {

template <typename Queue>
void runThroughput(std::int64_t consumerCount, std::int64_t itemsCount) {
    Queue q;
    std::atomic<std::int64_t> consumed{0};
    std::vector<std::uint64_t> checksums(
    static_cast<std::size_t>(consumerCount), 0);
    std::vector<std::thread> consumers;
    std::vector<std::vector<uint64_t>> per_thread_latencies(consumerCount);

    for (auto& v : per_thread_latencies){
        v.reserve(itemsCount);
    }

    for (std::int64_t i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&, i]() {
            std::uint64_t local = 0;
            while (consumed.load(std::memory_order_relaxed) < itemsCount) {
                unsigned int start_aux;
                unsigned int end_aux;

                uint64_t start =  __rdtscp(&start_aux);
                _mm_lfence();
                auto v = q.pop();
                
                uint64_t end = __rdtscp(&end_aux);
                _mm_lfence();
                if (v) {
                    per_thread_latencies[i].push_back(end-start);
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

 
    
    std::vector<uint64_t> all;
    all.reserve(itemsCount);
    

    for (auto& group : per_thread_latencies){
        all.insert(all.end(), group.begin(), group.end());
    }

    if (all.empty()) {
        std::cout << "  (no latency samples)\n";
        return;
    }
    
    std::sort(all.begin(), all.end());

    auto pct = [&](double p){
        return all[static_cast<int64_t>(all.size() * p)];
    };

    std::cout << "  consumers=" << consumerCount << '\n'
              << " items=" << itemsCount << '\n'
              << " secs=" << secs << '\n'
              << " items/sec=" << (static_cast<double>(itemsCount) / secs) << '\n'
              << " checksum=" << checksum << '\n'
              << " p99=" << pct(0.99) << '\n'
              << " p99.9=" << pct(0.999) << '\n'
              << " max=" << all.back() << '\n';
}

} // namespace queue_bench