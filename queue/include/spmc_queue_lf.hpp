
#include <vector>
#include <atomic>


using namespace std;
template <typename T, std::int64_t N>
class SPMCQueue{
private:
    struct Slot{
        std::atomic<int64_t> seq_;
        T data_;
    }

    std::array<Slot, N> buffer_;
    int64_t capacity_;
    int64_t size_;
    int64_t tail_;
    std::atomic<int64_t> head_;

public:

    SPMCQueue() : 
        capacity_{N}
        size_{0},
        tail_{0}
        head_{0}
    {   
        for (int i = 0; i < N; i++){
            buffer_[i].seq_.store({i, std::memory_order_relaxed});
        }
    }

    // can try getting this working
    SPMCQueue operator(SPMCQueue& other) = delete;
    SPMCQueue operator=(SPMCQueue& other) = delete;


    // didnt write a move constructor or assign

    bool push(const T& value){
        auto currTail = tail_;
        Slot& slot = buffer_[tail_ % capacity_];

        const std::size_t seqNum = slot.seq_.load(std::memory_order_acquire);

        if (currTail != seqNum){
            return false;
        }
        
        buffer_[currTail % capacity_].data_ = value; // might throw but not a problem, no state has changed
        slot.seq_.store(tail_ + 1, std::memory_order_release);
        
        tail_++;
        return true;
    }

    std::optional<T> pop(){
        while (true){
            auto currHead = head_.load(std::memory_order_relaxed);

            auto& slot = buffer_[currHead % capacity_];
            auto seqNum = slot.seq_.load(std::memory_order_acquire);

            
            if (seqNum < currHead + 1){
                return std::nullopt;
            }
            
            if (head_.compare_exchange_weak(
                currHead, 
                currHead + 1, 
                std::memory_order_relaxed, 
                std::memory_order_relaxed)){
                    T value_ = buffer_[currHead % capacity_].data_; // problem if this throws

                    slot.seq_.store(currHead + capacity_, std::memory_order_release);

                    return value_;
                }

        }
    }

};