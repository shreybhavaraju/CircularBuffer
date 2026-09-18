#include <deque>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

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

void test_copy_is_deep() {
    RingBuffer<int> a(3);
    for (int i = 1; i <= 4; ++i) a.push(i);  // wrapped: 2 3 4, head isn't at slot 0
    RingBuffer<int> b(a);
    CHECK(b.size() == 3);
    CHECK(b.capacity() == 3);
    CHECK(b[0] == 2 && b[1] == 3 && b[2] == 4);

    // changing one can't change the other, they each own their own array
    b.push(99);
    CHECK(a[0] == 2 && a[2] == 4);
    CHECK(b[0] == 3 && b[2] == 99);
    a.pop();
    CHECK(b.size() == 3);
}

void test_copy_keeps_wrapping_right() {
    // the copy starts unwrapped, check it still overwrites in the right order afterwards
    RingBuffer<int> a(4);
    for (int i = 0; i < 6; ++i) a.push(i);  // 2 3 4 5
    a.pop();                                // 3 4 5
    RingBuffer<int> b(a);
    b.push(6);
    b.push(7);  // full, overwrites 3
    CHECK(b.size() == 4);
    CHECK(b[0] == 4 && b[1] == 5 && b[2] == 6 && b[3] == 7);
}

void test_copy_assignment() {
    RingBuffer<std::string> a(2);
    a.push("x");
    a.push("y");
    RingBuffer<std::string> b(5);  // different capacity, gets replaced
    b.push("old");
    b = a;
    CHECK(b.capacity() == 2);
    CHECK(b.size() == 2);
    CHECK(b[0] == "x" && b[1] == "y");
    b.push("z");
    CHECK(a[0] == "x");  // a untouched
}

void test_self_assignment() {
    RingBuffer<int> a(3);
    a.push(1);
    a.push(2);
    RingBuffer<int>& same = a;  // through a reference so the compiler doesn't warn
    a = same;
    CHECK(a.size() == 2);
    CHECK(a[0] == 1 && a[1] == 2);
}

void test_move_constructor_steals_the_array() {
    RingBuffer<int> a(3);
    for (int i = 1; i <= 4; ++i) a.push(i);
    const int* before = &a[0];
    RingBuffer<int> b(std::move(a));
    CHECK(&b[0] == before);  // same memory, nothing was copied
    CHECK(b.size() == 3);
    CHECK(b[0] == 2 && b[2] == 4);

    // a is empty but still safe to use and destroy
    CHECK(a.empty());
    CHECK(!a.full());
    CHECK(a.capacity() == 0);
    CHECK_THROWS(a.pop(), std::out_of_range);
    CHECK_THROWS(a.push(1), std::logic_error);
}

void test_move_assignment() {
    RingBuffer<std::string> a(2);
    a.push("p");
    a.push("q");
    RingBuffer<std::string> b(4);
    b.push("gets freed");
    b = std::move(a);
    CHECK(b.capacity() == 2);
    CHECK(b[0] == "p" && b[1] == "q");
    CHECK(a.empty() && a.capacity() == 0);

    // a moved-from buffer can be assigned a new value and used again
    a = RingBuffer<std::string>(3);
    a.push("back");
    CHECK(a.front() == "back");
}

void test_copy_of_moved_from() {
    RingBuffer<int> a(2);
    RingBuffer<int> b(std::move(a));
    RingBuffer<int> c(a);
    CHECK(c.empty() && c.capacity() == 0);
}

void test_move_is_noexcept() {
    static_assert(std::is_nothrow_move_constructible_v<RingBuffer<int>>);
    static_assert(std::is_nothrow_move_assignable_v<RingBuffer<int>>);
}

// Counts how many Tracked objects are alive. new T[n] constructs n of them and delete[]
// destroys n, so after every buffer is gone the count has to be back at 0. If a delete[]
// is missing (leak) it stays above 0; if an array gets freed twice it goes negative.
struct Tracked {
    static int alive;
    static int constructed;
    int v = 0;
    Tracked() { ++alive; ++constructed; }
    Tracked(int x) : v(x) { ++alive; ++constructed; }
    Tracked(const Tracked& o) : v(o.v) { ++alive; ++constructed; }
    Tracked& operator=(const Tracked&) = default;
    ~Tracked() { --alive; }
};
int Tracked::alive = 0;
int Tracked::constructed = 0;

void test_every_element_destroyed_once() {
    Tracked::alive = 0;
    {
        RingBuffer<Tracked> a(4);
        CHECK(Tracked::alive == 4);  // new T[4] default constructs all 4 slots up front
        for (int i = 0; i < 10; ++i) a.push(Tracked(i));
        a.pop();
        RingBuffer<Tracked> b(a);        // +4
        RingBuffer<Tracked> c(2);        // +2
        c = b;                           // c's old 2 freed, +4 for the copy
        RingBuffer<Tracked> d(std::move(a));  // no new ones, d owns a's 4
        RingBuffer<Tracked> e(3);        // +3
        e = std::move(c);                // e's old 3 freed
        CHECK(Tracked::alive == 12);     // b, d, e own 4 each
    }
    CHECK(Tracked::alive == 0);
}

void test_move_doesnt_construct_anything() {
    RingBuffer<Tracked> a(1000);
    Tracked::constructed = 0;
    RingBuffer<Tracked> b(std::move(a));
    RingBuffer<Tracked> c(1);
    Tracked::constructed = 0;
    c = std::move(b);
    CHECK(Tracked::constructed == 0);
    RingBuffer<Tracked> d(c);  // a copy on the other hand makes 1000 new ones
    CHECK(Tracked::constructed == 1000);
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
    RUN(test_copy_is_deep);
    RUN(test_copy_keeps_wrapping_right);
    RUN(test_copy_assignment);
    RUN(test_self_assignment);
    RUN(test_move_constructor_steals_the_array);
    RUN(test_move_assignment);
    RUN(test_copy_of_moved_from);
    RUN(test_move_is_noexcept);
    RUN(test_every_element_destroyed_once);
    RUN(test_move_doesnt_construct_anything);

    if (g_failures) {
        std::printf("\n%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("\nall passed\n");
    return 0;
}
