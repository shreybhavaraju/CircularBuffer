CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Iinclude
# address sanitizer catches out of bounds reads/writes, use after free, double delete[]
# and (on linux) leaks. undefined behavior sanitizer catches things like % 0
SANITIZE = -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all

HEADERS = include/ring_buffer.hpp tests/check.hpp

test: build/tests
	./build/tests

# CI runs this on linux. on my mac asan hangs before main() even for hello world
# (ubsan alone is fine), so locally it's just make test
sanitize: build/tests_asan
	./build/tests_asan

demo: build/demo
	./build/demo

build/demo: examples/demo.cpp include/ring_buffer.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/tests: tests/test_ring_buffer.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/tests_asan: tests/test_ring_buffer.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $(SANITIZE) $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: test sanitize demo clean
