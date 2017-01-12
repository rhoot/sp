_PHONY: test

build:
	mkdir build

build/test: build tests/main.cpp tests/catch.hpp include/sp.hpp
	$(CXX) -std=c++11 -Wall -Werror -Wextra -g -O0 -o build/test tests/main.cpp

test: build/test
	build/test
