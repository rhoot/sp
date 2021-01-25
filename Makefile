_PHONY: test

build:
	mkdir -p build

build/test: build tests/main.cpp include/sp.hpp
	$(CXX) -o build/test -std=c++11 -Wall -Werror -Wextra -g -O0 $(CXXFLAGS) tests/main.cpp

test: build/test
	build/test
