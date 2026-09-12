CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Iinclude

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: clean
