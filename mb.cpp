#include <cstdint>
#include <cstdio>
#include <x86intrin.h>

int main() {
    volatile std::uint64_t capacity = 32768;
    std::uint64_t idx = 12345, sink = 0;
    const int N = 100000000;

    auto t0 = __rdtsc();
    for (int i = 0; i < N; ++i) sink += idx % capacity;
    auto t1 = __rdtsc();

    auto t2 = __rdtsc();
    for (int i = 0; i < N; ++i) sink += idx % 32768ULL;
    auto t3 = __rdtsc();

    auto t4 = __rdtsc();
    for (int i = 0; i < N; ++i) sink += capacity;
    auto t5 = __rdtsc();

    printf("load+divide : %.2f cyc/op\n", double(t1-t0)/N);
    printf("divide const: %.2f cyc/op\n", double(t3-t2)/N);
    printf("load only   : %.2f cyc/op\n", double(t5-t4)/N);
    printf("sink=%lu\n", sink);
    return 0;
}
