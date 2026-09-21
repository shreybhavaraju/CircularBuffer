# CircularBuffer

[![tests](https://github.com/shreybhavaraju/CircularBuffer/actions/workflows/ci.yml/badge.svg)](https://github.com/shreybhavaraju/CircularBuffer/actions/workflows/ci.yml)

A fixed-size circular buffer (ring buffer) in C++17. `RingBuffer<T>` owns one raw array allocated with `new T[capacity]` in the constructor and freed with `delete[]` in the destructor, no `std::vector` or smart pointers. `head` and `tail` indices wrap around with `% capacity`, and once the buffer is full a `push()` overwrites the oldest element, so it always holds the last `capacity` values. It's header-only ([include/ring_buffer.hpp](include/ring_buffer.hpp)) and implements the full rule of five, so copies are deep and moves just hand over the pointer.

```cpp
#include "ring_buffer.hpp"

RingBuffer<int> b(3);
for (int i = 1; i <= 5; ++i) b.push(i);  // 1 and 2 get overwritten
b.size();   // 3
b.front();  // 3 (oldest)
b.back();   // 5 (newest)
b[1];       // 4, indexed from the oldest
b.pop();    // 3
```

## How it works

```
make demo

start      [  .  .  .  . ]  size 0  H=0 T=0
push 10    [ 10  .  .  . ]  size 1  H=0 T=1
push 20    [ 10 20  .  . ]  size 2  H=0 T=2
push 30    [ 10 20 30  . ]  size 3  H=0 T=3
push 40    [ 10 20 30 40 ]  size 4  H=0 T=0  full
push 50    [ 50 20 30 40 ]  size 4  H=1 T=1  full
push 60    [ 50 60 30 40 ]  size 4  H=2 T=2  full
pop -> 30  [ 50 60  . 40 ]  size 3  H=3 T=2
pop -> 40  [ 50 60  .  . ]  size 2  H=0 T=2
push 70    [ 50 60 70  . ]  size 3  H=0 T=3
```

`head` (H) is the oldest element, `tail` (T) is where the next push goes. Both only ever move forward, `i = (i + 1) % capacity`, so after the last slot they go back to 0 and nothing ever gets shifted. When the buffer is full, head and tail point at the same slot, so a push writes over the oldest value and moves head along with tail (the `push 50` and `push 60` lines).

The one thing that tripped me up: `head == tail` is true both when the buffer is empty (`start`) and when it's full (`push 40`), so the indices alone can't tell you which. There's a separate `count`. (The other common fix is to waste one slot and call it full at `capacity - 1`, but then a buffer of 4 only holds 3.)

## Memory

The class owns a raw pointer, so the compiler-generated copy would copy the pointer and both objects would `delete[]` the same array. All five special members are written out:

| | what it does |
|---|---|
| destructor | `delete[] data_` |
| copy constructor | new array, copies elements oldest first so the copy starts unwrapped. If a `T` copy throws partway, frees the new array before rethrowing (the destructor doesn't run for a half-built object) |
| copy assignment | copy-and-swap: make the copy, then swap it in. If copying throws, the target is untouched, and self-assignment works |
| move constructor | takes the pointer and leaves the source with `nullptr` and capacity 0, no allocation. `noexcept`, so a `std::vector<RingBuffer>` moves them when it grows instead of copying |
| move assignment | frees its own array, takes the other one's, `noexcept` |

A moved-from buffer is empty with capacity 0. You can destroy it, copy it, check `size()`, or assign a new buffer to it. `push()` on it throws `std::logic_error` instead of doing `% 0`.

`pop()`, `front()`, `back()` on an empty buffer and `b[i]` with `i >= size()` throw `std::out_of_range` instead of reading a stale slot. `push(T&&)` and `pop()` move values instead of copying them.

## Tests

`make test` runs 21 tests ([tests/test_ring_buffer.cpp](tests/test_ring_buffer.cpp)) with a small `CHECK` macro instead of a test framework, so there's nothing to install:

- **fill / overflow / empty:** push to capacity, push past it and check the oldest ones got overwritten, pop everything and check the next pop throws. Also capacity 1, where every push overwrites.
- **wraparound:** 1,000 rounds of uneven pushes and pops so head and tail lap the array many times, with the full contents checked after every round against a `std::deque` doing the same thing the obvious way.
- **copy / move:** copies are independent of the original (including copies of wrapped buffers), self-assignment, assigning between different capacities, a move hands over the same memory, and the moved-from buffer is still safe to use.
- **lifetimes:** a `Tracked` type counts how many of it are alive. After a mix of copies, moves and assignments the count has to return to 0. I checked that removing the `delete[]` from the destructor makes this test fail. Another test checks that moving a 1,000-slot buffer constructs nothing while copying it constructs 1,000.

`make sanitize` builds the same tests with AddressSanitizer + UndefinedBehaviorSanitizer, which turn out-of-bounds access, use-after-free, double `delete[]` and leaks into crashes instead of silent bugs. CI runs `make test` on Linux (g++ and clang) and macOS, and `make sanitize` on Linux. On my Mac ASan hangs before `main()` even for a hello world (UBSan alone is fine), so I only run the sanitizer build in CI.

## Tradeoffs

- `new T[capacity]` default-constructs every slot up front, so `T` needs a default constructor, and a buffer of 1,000 strings makes 1,000 empty strings before you push anything. Popped and cleared values stay in their slots until they're overwritten or the buffer is destroyed. The way around both is raw storage (`operator new` + placement new + calling destructors manually). I kept `new[]`/`delete[]` because it keeps the ownership simple.
- Capacity is fixed at construction. There's no resize.
- Not thread-safe. A single-producer/single-consumer version would make head and tail `std::atomic` and drop `count` (and with it, use the "waste one slot" trick).

## Running it

```bash
make test       # build + run the tests
make sanitize   # same tests under asan + ubsan (linux)
make demo       # the walkthrough above
make clean
```

Needs a C++17 compiler (`g++` or `clang++`) and `make`. Pass `CXX=g++` or `CXX=clang++` to pick one.

## Layout

```
include/ring_buffer.hpp     the whole RingBuffer<T>
tests/test_ring_buffer.cpp  tests
tests/check.hpp             CHECK / CHECK_THROWS / RUN macros
examples/demo.cpp           make demo
Makefile
.github/workflows/ci.yml    tests on linux + mac, sanitizers on linux
```
