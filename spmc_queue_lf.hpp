
#include <vector>
#include <atomic>


template <typename T, typename N>
class SPMCQueue{
private:
    std::vector<T> buffer_;
    int64_t capacity_;
    int64_t size_;
    std::atomic<int64_t> tail_;
    std::atomic<int64_t> head_;
    

public:

    SPMCQueue(T type, N size){
        buffer_.resize(size);
        capacity_ = size;
        size_ = 0;
        tail_ = 0;
        head_ = 0;
    }

    // can try getting this working
    SPMCQueue operator(SPMCQueue& other) = delete;
    SPMCQueue operator=(SPMCQueue& other) = delete;


    // didnt write a move constructor or assign

    bool push(const T& value){
        auto currTail = tail_.load(std::memory_order_relaxed);
        if (currTail - head_.load(std::memory_order_acquire) == capacity_)
            return false;
        
        buffer_[currTail % capacity_] = value;
        tail_.store(currTail + 1, std::memory_order_release);

        return true;
    }

    std::optional<T> pop(){
        while (true){
            auto currHead = head_.load(std::memory_order_relaxed);

            if (currHead == tail_.load(std::memory_order_acquire)){
                return std::nullopt;
            }

            T value_ = buffer_[currHead % capacity_];

            if (head_.compare_exchange_weak(
                currHead, currHead + 1, 
                std::memory_order_release, std::memory_order_relaxed)){
                    return value_;
                }
        }
    }

};