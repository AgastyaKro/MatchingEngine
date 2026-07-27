#pragma once

#include <boost/lockfree/queue.hpp>

using namespace std;

template <typename T, int64_t N>
class BoostAdapterQueue{

private:
    boost::lockfree::queue<T, 
    boost::lockfree::capacity<N>> queue_;


public:

    bool push(const T& value){
        return queue_.push(value);
    }

    optional<T> pop(){
        T value;

        if (!queue_.pop(value))
            return std::nullopt;

        return value;

    }
};