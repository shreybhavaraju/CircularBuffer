#include <deque>
#include <stdexcept>
#include <string>

#include "check.hpp"
#include "ring_buffer.hpp"

void test_new_buffer_is_empty() {
    RingBuffer<int> b(4);
    CHECK(b.empty());
    CHECK(!b.full());
    CHECK(b.size() == 0);
    CHECK(b.capacity() == 4);
}

void test_zero_capacity_throws() {
    CHECK_THROWS(RingBuffer<int>(0), std::invalid_argument);
}

void test_fill() {
    RingBuffer<int> b(3);
    b.push(1);
    b.push(2);
    CHECK(!b.full());
    b.push(3);
    CHECK(b.full());
    CHECK(b.size() == 3);
    CHECK(b.front() == 1);
    CHECK(b.back() == 3);
}

void test_overflow_overwrites_oldest() {
    RingBuffer<int> b(3);
    for (int i = 1; i <= 5; ++i) b.push(i);  // 1 and 2 get overwritten
    CHECK(b.full());
    CHECK(b.size() == 3);
    CHECK(b[0] == 3);
    CHECK(b[1] == 4);
    CHECK(b[2] == 5);
    CHECK(b.front() == 3);
    CHECK(b.back() == 5);
}

void test_empty_it_out() {
    RingBuffer<int> b(3);
    for (int i = 1; i <= 5; ++i) b.push(i);
    CHECK(b.pop() == 3);  // oldest first
    CHECK(b.pop() == 4);
    CHECK(b.pop() == 5);
    CHECK(b.empty());
    CHECK_THROWS(b.pop(), std::out_of_range);
    CHECK_THROWS(b.front(), std::out_of_range);
    CHECK_THROWS(b.back(), std::out_of_range);
}

void test_capacity_one() {
    // every push overwrites, head and tail are always 0
    RingBuffer<int> b(1);
    b.push(7);
    b.push(8);
    CHECK(b.size() == 1);
    CHECK(b.front() == 8);
    CHECK(b.back() == 8);
    CHECK(b.pop() == 8);
    CHECK(b.empty());
}

void test_wraps_around_many_times() {
    // interleave pushes and pops so head and tail lap the array a lot, and compare every
    // step against a std::deque doing the same thing the slow obvious way
    RingBuffer<int> b(5);
    std::deque<int> expected;
    int next = 0;
    for (int round = 0; round < 1000; ++round) {
        int pushes = 1 + round % 4;  // 1 to 4 pushes, then 2 pops, so it fills up and drains
        for (int k = 0; k < pushes; ++k) {
            b.push(next);
            expected.push_back(next++);
            if (expected.size() > b.capacity()) expected.pop_front();  // overwritten
        }
        for (int k = 0; k < 2 && !expected.empty(); ++k) {
            CHECK(b.pop() == expected.front());
            expected.pop_front();
        }
        CHECK(b.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) CHECK(b[i] == expected[i]);
    }
}

void test_index_out_of_range_throws() {
    RingBuffer<int> b(4);
    b.push(1);
    b.push(2);
    CHECK(b[1] == 2);
    CHECK_THROWS(b[2], std::out_of_range);  // slot exists in the array but isn't in use
}

void test_clear() {
    RingBuffer<int> b(3);
    for (int i = 0; i < 4; ++i) b.push(i);
    b.clear();
    CHECK(b.empty());
    b.push(9);
    CHECK(b.front() == 9);
    CHECK(b.size() == 1);
}

void test_works_with_strings() {
    RingBuffer<std::string> b(2);
    b.push("a");
    b.push("b");
    b.push("c");
    CHECK(b.pop() == "b");
    CHECK(b.pop() == "c");
}

int main() {
    RUN(test_new_buffer_is_empty);
    RUN(test_zero_capacity_throws);
    RUN(test_fill);
    RUN(test_overflow_overwrites_oldest);
    RUN(test_empty_it_out);
    RUN(test_capacity_one);
    RUN(test_wraps_around_many_times);
    RUN(test_index_out_of_range_throws);
    RUN(test_clear);
    RUN(test_works_with_strings);

    if (g_failures) {
        std::printf("\n%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("\nall passed\n");
    return 0;
}
