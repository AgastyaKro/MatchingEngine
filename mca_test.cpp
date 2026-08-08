#include <cstdint>

std::uint64_t test(std::uint64_t idx, volatile std::uint64_t capacity) {
    std::uint64_t sink = 0;
    for (int i = 0; i < 1000; ++i) {
        __asm__ volatile("# LLVM-MCA-BEGIN divloop");
        sink += idx % capacity;
        __asm__ volatile("# LLVM-MCA-END");
    }
    return sink;
}
