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
#include "catch/catch.hpp"

#include <cfloat> // DBL_MAX, FLT_MIN, FLT_MAX
#include <cmath> // NAN, INFINITY

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
        REQUIRE(std::strcmp(expected, buffer) == 0);                                     \
        REQUIRE(expectedLen == actualLen);                                               \
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

struct Foo {
};

static bool format_value(sp::Output& output, const sp::StringView& format, const Foo&)
{
    if (!format.length) {
        output.write(7, "<empty>");
    } else {
        output.write(format.length, format.ptr);
    }
    return true;
}

TEST_CASE("Custom format")
{
    TEST_FORMAT("<@:>f0\\", "{:<@:>f0\\}", Foo{});
    TEST_FORMAT("{:{}", "{:{}}", Foo{});
    TEST_FORMAT("<empty>}", "{:}}", Foo{});
}

TEST_CASE("Readme formats")
{
    // Examples
    TEST_FORMAT("Hello, World!\n", "Hello, {}!\n", "World");
    TEST_FORMAT("+0000512", "{:+08}", 512);
    TEST_FORMAT("name=John,height=1.80,employed=true", "name={2},height={0:.2f},employed={1}", 1.8019f, true, "John");

    // Format string
    TEST_FORMAT("{:", "{:", 1);
    TEST_FORMAT("{:.}", "{:.}", 1);
    TEST_FORMAT("foo", "{:=}", "foo");
    TEST_FORMAT("{0!s}", "{0!s}", 1);
    TEST_FORMAT("{0!a}", "{0!a}", 1);
    TEST_FORMAT("0 2", "{} {2}", 0, 1, 2);
    TEST_FORMAT("{foo.bar}", "{foo.bar}", 1);
    TEST_FORMAT("{0[0]}", "{0[0]}", 1);
    TEST_FORMAT("0 1 2", "{} {} {}", 0, 1, 2);
    TEST_FORMAT("0 1 2", "{0} {1} {2}", 0, 1, 2);
    TEST_FORMAT("0 1 1 2 1", "{} {} {1} {} {1}", 0, 1, 2);
    TEST_FORMAT("0 1 1 2 1", "{0} {1} {1} {2} {1}", 0, 1, 2);
    TEST_FORMAT("{0:{1}", "{0:{1}}", 1);
    TEST_FORMAT("{:_}", "{:_}", 1);
    TEST_FORMAT("{:,}", "{:,}", 1);
    TEST_FORMAT("{:c}", "{:c}", 'f');
    TEST_FORMAT("314159265", "{}", 314159265.0);
}

TEST_CASE("Integer formats")
{
    TEST_FORMAT("true", "{}", true);
    TEST_FORMAT("false", "{}", false);

    TEST_FORMAT("1", "{}", (char)1);
    TEST_FORMAT("1", "{}", (signed char)1);
    TEST_FORMAT("1", "{}", (unsigned char)1);
    TEST_FORMAT("1", "{}", (short)1);
    TEST_FORMAT("1", "{}", (unsigned short)1);
    TEST_FORMAT("1", "{}", (int)1);
    TEST_FORMAT("1", "{}", (unsigned)1);
    TEST_FORMAT("1", "{}", (long)1);
    TEST_FORMAT("1", "{}", (unsigned long)1);
    TEST_FORMAT("1", "{}", (long long)1);
    TEST_FORMAT("1", "{}", (unsigned long long)1);

    TEST_FORMAT("1", "{}", int8_t(1));
    TEST_FORMAT("1", "{}", uint8_t(1));
    TEST_FORMAT("1", "{}", int16_t(1));
    TEST_FORMAT("1", "{}", uint16_t(1));
    TEST_FORMAT("1", "{}", int32_t(1));
    TEST_FORMAT("1", "{}", uint32_t(1));
    TEST_FORMAT("1", "{}", int64_t(1));
    TEST_FORMAT("1", "{}", uint64_t(1));

    TEST_FORMAT("7ff00000", "{}", (void*)0x7ff00000);

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

TEST_CASE("Float formats")
{
    TEST_FORMAT("1", "{}", 1.0);
    TEST_FORMAT("1.5", "{}", 1.5f);
    TEST_FORMAT("1.79769313486232e+308", "{}", DBL_MAX);
    TEST_FORMAT("1.17549e-38", "{}", FLT_MIN);
    TEST_FORMAT("-3.40282e+38", "{}", -FLT_MAX);

    TEST_FORMAT(" 1.000000e+00", "{: e}", 1.0f);
    TEST_FORMAT("-1.000000e+00", "{:e}", -1.0f);
    TEST_FORMAT("1.234568E+05", "{:E}", 123456.789);
    TEST_FORMAT("5.12E+02", "{:.2E}", 512.1024);
    TEST_FORMAT("3.251923299534e+01", "{:.12e}", 32.5192329953432345);

    TEST_FORMAT("1.000000", "{:f}", 1.0f);
    TEST_FORMAT("-1.000000", "{:f}", -1.0f);
    TEST_FORMAT("+1.234568", "{:+f}", 1.23456789f);
    TEST_FORMAT("3.1416", "{:.4f}", 3.14159265f);
    TEST_FORMAT("1.57079633", "{:.8f}", 1.5707963267948966192);

    TEST_FORMAT("1", "{:g}", 1.0);
    TEST_FORMAT("-52", "{:g}", -52.0f);
    TEST_FORMAT("3.14", "{:G}", 3.14);
    TEST_FORMAT("+3.142", "{:+.4g}", 3.14159265);
    TEST_FORMAT("1.23457e+19", "{:.6g}", 12345678901234567890.0);

    TEST_FORMAT("   12", "{:5g}", 12.0f);
    TEST_FORMAT("42.0101  ", "{:<9.6g}", 42.0101);
    TEST_FORMAT("xxx32.007", "{:x>9.3f}", 32.00723f);
    TEST_FORMAT("__1__", "{:_^5g}", 1.0f);
    TEST_FORMAT("??2???", "{:?^6g}", 2.0f);
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
