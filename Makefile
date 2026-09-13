CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Iinclude

HEADERS = include/ring_buffer.hpp tests/check.hpp

test: build/tests
	./build/tests

build/tests: tests/test_ring_buffer.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: test clean
