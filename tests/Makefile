_PHONY: test

tester: main.cpp catch.hpp
	g++ -std=c++11 -Wall -Werror -Wextra -g -O0 -o tester main.cpp

test: tester
	./tester
