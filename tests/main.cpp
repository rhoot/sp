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

#define TEST_FORMAT(expected, fmt, ...)                                                  \
    for (;;) {                                                                           \
        auto buffer = (char*)std::malloc(10 * 1024 * 1024);                              \
        buffer[0] = 0;                                                                   \
        const auto expectedLen = int32_t(std::strlen(expected));                         \
        const auto actualLen = sp::format(buffer, 10 * 1024 * 1024, fmt, ##__VA_ARGS__); \
        REQUIRE(expectedLen == actualLen);                                               \
        REQUIRE(std::strcmp(expected, buffer) == 0);                                     \
        std::free(buffer);                                                               \
        break;                                                                           \
    }

TEST_CASE("Unformatted")
{
    TEST_FORMAT("", "");
    TEST_FORMAT("foo", "foo");
    TEST_FORMAT("{", "{{");
    TEST_FORMAT("}", "}}");
    TEST_FORMAT("{}", "{{}}");
    TEST_FORMAT("}{", "}}{{");
    TEST_FORMAT("{0}", "{{0}}", 1);
    TEST_FORMAT("{{0}}", "{{{{0}}}}", 1);
    TEST_FORMAT("a{b", "a{{b");
    TEST_FORMAT("a}b", "a}}b");
}

TEST_CASE("Integer formats")
{
    TEST_FORMAT("42", "{}", 42);
    TEST_FORMAT("-15", "{0}", -15);
    TEST_FORMAT("+96", "{:+}", 96);
    TEST_FORMAT(" 7", "{: }", 7);
    TEST_FORMAT(" 32", "{: 3}", 32);
    TEST_FORMAT("  75", "{: 4}", 75);
    TEST_FORMAT("   4", "{:4}", 4);
    TEST_FORMAT("300", "{:0<3}", 3);
    TEST_FORMAT(" 2  ", "{:^4}", 2);
    TEST_FORMAT("  8  ", "{:^5}", 8);
    TEST_FORMAT("+  52", "{:=+5}", 52);
    TEST_FORMAT("101000", "{:b}", 40);
    TEST_FORMAT("0b1000100", "{:#b}", 68);
    TEST_FORMAT("77", "{:o}", 63);
    TEST_FORMAT("0o36", "{:#o}", 30);
    TEST_FORMAT("abc", "{:x}", 2748);
    TEST_FORMAT("0xba", "{:#x}", 186);
    TEST_FORMAT("F00", "{:X}", 3840);
    TEST_FORMAT("0XBAD", "{:#X}", 2989);

    TEST_FORMAT("-0b10000000", "{:#b}", INT8_MIN);
    TEST_FORMAT("+  177", "{:=+6o}", INT8_MAX);
    TEST_FORMAT(">> 18446744073709551615", "{:>> 23}", UINT64_MAX);
    TEST_FORMAT("0x7fffffffffffffff", "{:#x}", INT64_MAX);
}

TEST_CASE("String formats")
{
    TEST_FORMAT("abc", "a{}c", "b");
    TEST_FORMAT("bar", "{}{}{}", "b", "a", "r");
    TEST_FORMAT("baz", "{2}{0}{}", "a", "z", "b");
    TEST_FORMAT("{foo}", "{{{}}}", "foo");
    TEST_FORMAT("foo ", "{:4}", "foo");
    TEST_FORMAT("foo", "{:o<3}", "f");
    TEST_FORMAT(".foo", "{:.>4}", "foo");
    TEST_FORMAT("  foo  ", "{:^7}", "foo");
    TEST_FORMAT("  foo   ", "{:^8}", "foo");
    TEST_FORMAT("cc", "{:c<2s}", "c");
    TEST_FORMAT("trunc", "{:.5}", "truncate");
    TEST_FORMAT("--ball---", "{:-^9.4s}", "ballet");

    std::string str(999, ' ');
    str.push_back('a');
    TEST_FORMAT(str.data(), "{0:>1000}", "a");

    str = std::string(1000, ' ');
    TEST_FORMAT(str.data(), "{0:1000}", "");
}
