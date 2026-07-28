#pragma once

#include <boost/lockfree/queue.hpp>


template <typename T, std::int64_t N>
class BoostAdapterQueue{

private:
    boost::lockfree::queue<T, 
    boost::lockfree::capacity<N>> queue_;

public:

    bool push(const T& value){
        return queue_.push(value);
    }

    std::optional<T> pop(){
        T value;

        if (!queue_.pop(value))
            return std::nullopt;

        return value;

    }
};