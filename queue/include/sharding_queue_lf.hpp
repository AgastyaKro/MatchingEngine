#include <cstdint>
#include <array>
#include <atomic>


template <typename T, std::uint64_t Consumers, std::uint64_t Size>
class ShardingQueue{

    class InternalQueue{
    private:
        alignas(64) std::atomic<uint64_t> head_;
        alignas(64) std::atomic<uint64_t> tail_;
        std::array<T, Size> buffer_;

    public:

        std::optional<T> pop(){


        }

        bool push(const T& t){

        }


    };

private:
    std::array<InternalQueue, Consumers> shardingQueue_;
    
public:

    std::optional<T> pop(uint64_t consumerQueue){

    }

    bool push(const T& t, uint64_t consumerQueue){

    }
};