// Prints the raw array after every push/pop so you can watch head (H) and tail (T) move
// and wrap around. Slots that aren't in use show as '.'.
//   make demo

#include <cstdio>

#include "ring_buffer.hpp"

// RingBuffer doesn't expose its slots (and shouldn't), so this rebuilds the picture from
// what it does expose: the oldest element sits at some slot `head` and the rest follow
void show(const char* what, const RingBuffer<int>& b, std::size_t head) {
    std::size_t cap = b.capacity();
    std::size_t tail = (head + b.size()) % cap;
    std::printf("%-10s [", what);
    for (std::size_t slot = 0; slot < cap; ++slot) {
        std::size_t offset = (slot + cap - head) % cap;  // how far this slot is from head
        if (offset < b.size()) std::printf(" %2d", b[offset]);
        else std::printf("  .");
    }
    std::printf(" ]  size %zu  H=%zu T=%zu%s\n", b.size(), head, tail, b.full() ? "  full" : "");
}

int main() {
    RingBuffer<int> b(4);
    std::size_t head = 0;  // tracked alongside the buffer, just for printing
    show("start", b, head);

    for (int i = 1; i <= 6; ++i) {
        bool overwrites = b.full();
        b.push(i * 10);
        if (overwrites) head = (head + 1) % b.capacity();
        char label[16];
        std::snprintf(label, sizeof label, "push %d", i * 10);
        show(label, b, head);
    }
    for (int i = 0; i < 2; ++i) {
        int v = b.pop();
        head = (head + 1) % b.capacity();
        char label[16];
        std::snprintf(label, sizeof label, "pop -> %d", v);
        show(label, b, head);
    }
    b.push(70);
    show("push 70", b, head);
}
