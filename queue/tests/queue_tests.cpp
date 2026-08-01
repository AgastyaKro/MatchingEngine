#include "boost_queue_adapter.hpp"
#include "queue_benchmark.hpp"
#include "queue_test_suite.hpp"
#include "spmc_queue_lf.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {
    constexpr std::int64_t kTestCapacity = 1024;
    constexpr std::int64_t kBenchCapacity = 32'768;

    template <typename T>
    struct QueueTraits{
        using SPMCTest = SPMCQueue<T, kTestCapacity>;
        using BoostTest = BoostAdapterQueue<T, kTestCapacity>;
        using SPMCBench = SPMCQueue<T, kBenchCapacity>;
        using BoostBench = BoostAdapterQueue<T, kBenchCapacity>;
    };

    template <typename T>
    void correctnessFor(std::string_view typeName){
        using Q = QueueTraits<T>;
        std::cerr << "== correctness [" << typeName << "] == \n";
        std::cerr << "SPMCQueue... \n";
        queue_tests::runCorrectnessTests<typename Q::SPMCTest, kTestCapacity>();
        std::cerr << "SPMCQueue passed \n";
        std::cerr << "Boost... \n";
        queue_tests::runCorrectnessTests<typename Q::BoostTest, kTestCapacity>();
        std::cerr << "BoostQueue passed \n";
    }

    template<typename T>
    void benchFor(std::string_view typeName, std::string_view which, std::int64_t consumers, std::int64_t items){
        using Q = QueueTraits<T>;

        std::cerr << "== benchmark[" << typeName << "] (consumers=" << consumers << " items=" << items << ") ==\n";

        if (which.empty() || which == "spmc"){
            std::cout << "SPMCQueue [" << typeName << "]:\n";
            queue_bench::runThroughput<typename Q::SPMCBench>(consumers, items);
        }
        if (which.empty() || which == "boost"){
            std::cout << "BoostQueue [" << typeName << "]:\n";
            queue_bench::runThroughput<typename Q::BoostBench>(consumers, items);
        }
    }

    void printUsage(){
        std::cerr 
        << "Usage:\n"
        << " ./queue_tests correctness\n"
        << " ./queue_tests bench [spmc|boost] [consumers] [items]\n";
    }


    std::int64_t parseInt(std::string_view name, const char* text){
        try {
            return static_cast<std::int64_t>(std::stoll(text));
        }
        catch(const std::exception&){
            throw std::runtime_error(
                "bad " + std::string(name) + " arguments: '" + std::string(text) + "'");
        }

    }


}

int main(int argc, char** argv){
    if (argc < 2){
        printUsage();
        return 1;
    }

    const std::string_view mode = argv[1];

    try{
        if (mode == "correctness"){
            correctnessFor<std::uint32_t>("uint32_t");
            correctnessFor<std::uint64_t>("uint64_t");
            return 0;
        }
        if (mode == "bench"){
            std::string_view which;
            if (argc >= 3){
                which = argv[2];
                if (which != "spmc" && which != "boost"){
                    std::cerr << "unknown queue" << "\n";
                    printUsage();
                    return 1;
                }
            }

            const std::int64_t consumers = argc >= 4 ? parseInt("consumers", argv[3]) : 3;
            const std::int64_t items = argc >= 5 ? parseInt("items", argv[4]) : 1'000'000;

            if (consumers <= 0){
                std::cerr << "consumers must be > 0 \n";
                return 1;
            }

            benchFor<std::uint32_t>("u32", which, consumers, items);
            benchFor<std::uint64_t>("u64", which, consumers, items);
            return 0;
        }

        std::cerr << "unknown mode: " << mode << "\n";
        printUsage();
        return 1;
    }
    catch(const std::exception& e){
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
    catch(...){
        std::cerr << "FAILED: unknown exception" << '\n';
        return 1;
    }
}

// ./queue_tests bench [spmc|boost] [consumers] [items]