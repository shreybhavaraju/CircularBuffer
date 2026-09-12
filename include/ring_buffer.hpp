// RingBuffer<T>: a fixed-size circular buffer that owns a raw array.
// The array is allocated once in the constructor with new T[capacity] and freed once in
// the destructor with delete[]. No std::vector, so all of the memory handling is in here.

#pragma once

#include <cstddef>
#include <stdexcept>

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("RingBuffer capacity has to be at least 1");
        }
        data_ = new T[capacity_];
    }

    ~RingBuffer() { delete[] data_; }

    std::size_t capacity() const { return capacity_; }
    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == capacity_; }

private:
    T* data_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t count_ = 0;
};
