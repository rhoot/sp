// sp - string formatting micro-library
//
// Written in 2017 by Johan Sköld
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the public
// domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../include/sp.hpp"

TEST_CASE("Output with a string buffer")
{
    SECTION("it should result in the length of the written string")
    {
        char buffer[64];
        sp::Output output(buffer, sizeof(buffer));
        output.write(3, "foo");
        REQUIRE(output.result() == 3);
        output.write('d');
        REQUIRE(output.result() == 4);
    }

    SECTION("it should not overflow")
    {
        char buffer[6] = { -1, -1, -1, -1, -1, -1 };

        // Lie about the length so we can make sure it doesn't overflow
        sp::Output output(buffer, 4);
        output.write(6, "foobar");

        REQUIRE(buffer[0] == 'f');
        REQUIRE(buffer[1] == 'o');
        REQUIRE(buffer[2] == 'o');
        REQUIRE(buffer[3] == 0);
        REQUIRE(buffer[4] == -1);
        REQUIRE(buffer[5] == -1);
    }

    SECTION("it should result in the length of the full string, even when truncated")
    {
        char buffer[5];

        sp::Output output(buffer, sizeof(buffer));
        output.write(10, "ooga booga");

        REQUIRE(buffer[0] == 'o');
        REQUIRE(buffer[1] == 'o');
        REQUIRE(buffer[2] == 'g');
        REQUIRE(buffer[3] == 'a');
        REQUIRE(buffer[4] == 0);

        REQUIRE(output.result() == 10);
    }
}

template <class... Args>
static void test_format(const char expected[], const char fmt[], Args&&... args)
{
    char buffer[1024] = { 0 };
    const int32_t expectedLen = std::strlen(expected);
    const int32_t actualLen = sp::format(buffer, fmt, std::forward<Args>(args)...);
    REQUIRE(expectedLen == actualLen);
    REQUIRE(std::strcmp(expected, buffer) == 0);
}

TEST_CASE("python format tests")
{
    test_format("", "");
    test_format("a", "a");
    test_format("ab", "ab");
    test_format("a{", "a{{");
    test_format("a}", "a}}");
    test_format("{b", "{{b");
    test_format("}b", "}}b");
    test_format("a{b", "a{{b");

    // test_format("My name is Fred", "My name is {0}", "Fred");
    // test_format("My name is Fred :-{}", "My name is {0} :-{{}}", "Fred");
}