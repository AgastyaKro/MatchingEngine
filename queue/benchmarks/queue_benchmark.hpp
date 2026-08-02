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
#include <hdr/hdr_histogram.h>

namespace queue_bench {

template <typename Queue>
void runThroughput(std::int64_t consumerCount, std::int64_t itemsCount) {
    Queue q;
    std::atomic<std::int64_t> consumed{0};
    std::vector<std::uint64_t> checksums(
    static_cast<std::size_t>(consumerCount), 0);
    std::vector<std::thread> consumers;

    std::vector<hdr_histogram*> hists(consumerCount, nullptr);

    
    for (auto& hist : hists){
        hdr_init(1, 10'000'000, 3, &hist);
    }

    for (std::int64_t i = 0; i < consumerCount; ++i) {
        consumers.emplace_back([&, i]() {
            std::uint64_t local = 0;
            hdr_histogram* h = hists[i];
            while (consumed.load(std::memory_order_relaxed) < itemsCount) {
                uint64_t start =  __rdtsc();
                auto v = q.pop();
                uint64_t dt = __rdtsc() - start;
                if (v) {
                    hdr_record_value(h, dt);
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


    hdr_histogram* merged = nullptr;
    hdr_init(1, 10'000'000, 3, &merged);

    for (auto h : hists){
        hdr_add(merged, h);
    }

    std::cout << " consumers=" << consumerCount << '\n'
              << " items=" << itemsCount << '\n'
              << " secs=" << secs << '\n'
              << " items/sec=" << (static_cast<double>(itemsCount) / secs) << '\n'
              << " checksum=" << checksum << '\n'
              << " latency (TSC cycles): "
              << " p99=" << hdr_value_at_percentile(merged,50.0) << '\n'
              << " p99.9=" << hdr_value_at_percentile(merged,99.0) << '\n'
              << " max=" << hdr_max(merged) << '\n';
    
    for (auto h : hists){
        hdr_close(h);
    }
    hdr_close(merged);
}

} // namespace queue_bench