#include <cstdint>
#include <array>
#include <atomic>
#include <optional>


template <typename T, std::uint64_t Consumers, std::uint64_t Size>
class ShardingQueue{
    static_assert((Size & (Size-1)) == 0, "Size must be a power of two");
    static_assert((Consumers & (Consumers-1)) == 0, "Consumers must be a power of two");


public:
    class InternalQueue{
    private:
        alignas(64) std::atomic<uint64_t> head_;
        alignas(64) std::atomic<uint64_t> tail_;
        std::array<T, Size> buffer_;

    public:
        InternalQueue() : head_(0), tail_(0) {}

        std::optional<T> pop(){
            uint64_t head = head_.load(std::memory_order_relaxed);
            if (head == tail_.load(std::memory_order_acquire))
                return std::nullopt;
            
            T value = buffer_[head & (Size-1)];
            head_.store(head+1, std::memory_order_release);
            return value;
        }

        bool push(const T& t){
            uint64_t tail = tail_.load(std::memory_order_relaxed);
            if (tail - head_.load(std::memory_order_acquire) == Size)
                return false;
            buffer_[tail & (Size - 1)] = t;
            tail_.store(tail + 1, std::memory_order_release);
            return true;
        }


    };

    ShardingQueue() : counter_(0) {}
    ShardingQueue(const ShardingQueue&) = delete;
    ShardingQueue& operator=(const ShardingQueue&) = delete;


    std::optional<T> pop(std::uint64_t consumerQueue){
        return shardingQueue_[consumerQueue].pop();
    }

    bool push(const T& t){
        for (uint64_t i = 0; i < Consumers; ++i){
            if (shardingQueue_[(counter_ + i) & (Consumers - 1)].push(t)){
                counter_++;
                return true;
            }
        }
        return false;
    }

    InternalQueue* getShard(std::uint64_t consumerQueue){
        return &shardingQueue_[consumerQueue];
    }

private:
    std::array<InternalQueue, Consumers> shardingQueue_;
    alignas(64) std::uint64_t counter_;
};