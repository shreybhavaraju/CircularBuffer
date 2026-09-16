// RingBuffer<T>: a fixed-size circular buffer that owns a raw array.
// The array is allocated once in the constructor with new T[capacity] and freed once in
// the destructor with delete[]. No std::vector, so all of the memory handling is in here.
//
//   head_  - index of the oldest element, the one pop() returns next
//   tail_  - index where the next push() writes
//   count_ - how many elements are stored. Needed because head_ == tail_ both when the
//            buffer is empty and when it's full, so the indices alone can't tell them apart
//
// Both indices move forward with (i + 1) % capacity, so after the last slot they wrap back
// to slot 0. When the buffer is full, push() overwrites the oldest element and moves head_
// forward with it, so the buffer always holds the most recent `capacity` pushes.

#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

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

    // Copy constructor: deep copy. The default one would copy the pointer, and then both
    // buffers would delete[] the same array. Elements are copied oldest first into slot 0,
    // 1, ... so the copy is "unwrapped" (head_ = 0) but holds the same sequence.
    RingBuffer(const RingBuffer& other)
        : capacity_(other.capacity_), count_(other.count_) {
        data_ = new T[capacity_];
        try {
            for (std::size_t i = 0; i < count_; ++i) {
                data_[i] = other[i];
            }
        } catch (...) {
            // T's copy threw partway through. The destructor won't run for an object whose
            // constructor didn't finish, so free the array here or it leaks
            delete[] data_;
            throw;
        }
        tail_ = count_ % capacity_;
    }

    // Copy assignment, copy-and-swap: build the copy first, then swap it in. If the copy
    // throws, *this hasn't been touched yet. Our old array ends up in `copy` and gets
    // freed by its destructor at the end of the function.
    RingBuffer& operator=(const RingBuffer& other) {
        if (this != &other) {
            RingBuffer copy(other);
            swap(copy);
        }
        return *this;
    }

    void swap(RingBuffer& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(capacity_, other.capacity_);
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(count_, other.count_);
    }

    // Adds to the back. If the buffer is full the oldest element gets overwritten.
    void push(const T& value) {
        data_[tail_] = value;
        tail_ = (tail_ + 1) % capacity_;
        if (full()) {
            head_ = (head_ + 1) % capacity_;  // the slot we just wrote was the oldest one
        } else {
            ++count_;
        }
    }

    // Removes and returns the oldest element.
    T pop() {
        if (empty()) {
            throw std::out_of_range("pop() on an empty RingBuffer");
        }
        T value = data_[head_];
        head_ = (head_ + 1) % capacity_;
        --count_;
        return value;
    }

    // oldest and newest element, without removing them
    const T& front() const {
        if (empty()) throw std::out_of_range("front() on an empty RingBuffer");
        return data_[head_];
    }
    const T& back() const {
        if (empty()) throw std::out_of_range("back() on an empty RingBuffer");
        return data_[(tail_ + capacity_ - 1) % capacity_];  // + capacity_ so it can't go below 0
    }

    // i-th element counting from the oldest, so buf[0] == front() and buf[size() - 1] == back()
    const T& operator[](std::size_t i) const {
        if (i >= count_) throw std::out_of_range("RingBuffer index out of range");
        return data_[(head_ + i) % capacity_];
    }

    void clear() {
        head_ = tail_ = count_ = 0;
    }

    std::size_t capacity() const { return capacity_; }
    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == capacity_; }

private:
    T* data_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t count_ = 0;
};
