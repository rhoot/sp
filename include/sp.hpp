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

#include <cstdint> // int32_t, uint64_t
#include <cstdio> // printf, FILE, fwrite, fprintf
#include <cstring> // std::memcpy
#include <algorithm> // std::min, std::max
#include <utility> // std::forward

///
// API
///

namespace sp {

    class Output {
    public:
        Output();
        Output(char buffer[], size_t size);
        explicit Output(FILE* stream);

        Output(const Output&) = delete;
        Output& operator=(const Output&) = delete;

        int32_t result() const;
        void write(char ch);
        void write(size_t length, const void* data);

    private:
        FILE* m_stream = nullptr;
        char* m_buffer = nullptr;
        int32_t m_size = 0;
        int32_t m_length = 0;
    };

    template <class... Args>
    int32_t print(const char fmt[], Args&&... args);

    template <class... Args>
    int32_t format(Output& output, const char fmt[], Args&&... args);

    template <class... Args>
    int32_t format(FILE* file, const char fmt[], Args&&... args);

    template <class... Args>
    int32_t format(char buffer[], size_t size, const char fmt[], Args&&... args);

    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const char fmt[], Args&&... args);

} // namespace sp

///
// Implementation
///

namespace sp {

    inline Output::Output()
        : m_stream(stdout)
    {
    }

    inline Output::Output(char buffer[], size_t size)
        : m_buffer(buffer)
        , m_size(int32_t(size))
    {
    }

    inline Output::Output(FILE* stream)
        : m_stream(stream)
    {
    }

    inline int32_t Output::result() const
    {
        return m_length;
    }

    inline void Output::write(char ch)
    {
        write(sizeof(ch), &ch);
    }

    inline void Output::write(size_t bytes, const void* data)
    {
        if (m_length >= 0) {
            if (m_stream) {
                const auto written = std::fwrite(data, 1, bytes, m_stream);

                if (written == bytes) {
                    m_length += int32_t(written);
                } else {
                    m_length = -1;
                }
            } else {
                m_length += int32_t(bytes);

                if (m_size) {
                    const auto toCopy = std::min(size_t(m_size) - 1, bytes);
                    std::memcpy(m_buffer, data, toCopy);
                    m_buffer[toCopy] = 0;
                    m_buffer += toCopy;
                    m_size -= int32_t(toCopy);
                }
            }
        }
    }

    struct FormatFlags {
        char fill = 0;
        char align = 0;
        char sign = 0;
        bool alternate = false;
        int32_t width = -1;
        int32_t precision = -1;
        char type = 0;
    };

    bool format_value(Output&, const FormatFlags&, char32_t)
    {
        return false;
    }

    bool format_int(Output& output, const FormatFlags& flags, bool isNegative, uint64_t value)
    {
        if (flags.type == 'c') {
            return !isNegative && format_value(output, flags, char32_t(value));
        }

        // determine base
        int32_t base = 10;

        switch (flags.type) {
        case 'b':
            base = 2;
            break;
        case 'o':
            base = 8;
            break;
        case 'x':
        case 'X':
            base = 16;
            break;
        }

        // count digits, and copy them to a buffer (so we don't have to repeat
        // this later)
        char buffer[66]; // max needed; 64-bit binary with alternate form
        char* digits = buffer + sizeof(buffer);
        int32_t ndigits = 0;

        {
            const auto digitchars = (flags.type == 'X')
                ? "0123456789ABCDEF"
                : "0123456789abcdef";

            uint64_t v = value;
            do {
                *(--digits) = digitchars[v % base];
                v /= base;
                ++ndigits;
            } while (v);

            if (flags.alternate) {
                switch (base) {
                case 2:
                    *(--digits) = 'b';
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                case 8:
                    *(--digits) = 'o';
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                case 16:
                    *(--digits) = flags.type;
                    *(--digits) = '0';
                    ndigits += 2;
                    break;
                }
            }
        }

        // determine sign
        char sign = 0;

        if (isNegative) {
            sign = '-';
        } else if (flags.sign == '+' || flags.sign == ' ') {
            sign = flags.sign;
        }

        // determine spacing
        int32_t nchars = sign ? ndigits + 1 : ndigits;
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;
        {
            int32_t width = std::max(flags.width, nchars);

            switch (flags.align) {
            case '^':
                leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
                tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
                leadSpace += (width - nchars - 1) & 1; // if both are even or both are odd, we need to add one for correction
                tailSpace -= (width - nchars - 1) & 1; // if both are even or both are odd, we need to remove one for correction
                break;
            case '<':
                tailSpace = width - nchars;
                break;
            case '>':
            case '=':
            default:
                leadSpace = width - nchars;
                break;
            }
        }

        // print sign, if it should be before the padding
        if (sign && flags.align == '=') {
            output.write(sign);
        }

        // apply the leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            output.write(fill);
        }

        // print the sign, if it should be after the padding
        if (sign && flags.align != '=') {
            output.write(sign);
        }

        // print digits
        output.write(ndigits, digits);

        // print tailing padding
        while (tailSpace--) {
            output.write(fill);
        }

        return true;
    }

    bool format_value(Output& output, const FormatFlags& flags, uint64_t value)
    {
        return format_int(output, flags, false, value);
    }

    bool format_value(Output& output, const FormatFlags& flags, int64_t value)
    {
        // negating INT64_MIN is undefined behavior; the result is out of range of
        // a signed 64-bit int
        if (value != INT64_MIN) {
            const auto abs = (value < 0) ? uint64_t(-value) : uint64_t(value);
            return format_int(output, flags, value < 0, abs);
        } else {
            return format_int(output, flags, true, uint64_t(value));
        }
    }

    bool format_value(Output& output, const FormatFlags& flags, uint32_t value)
    {
        return format_value(output, flags, uint64_t(value));
    }

    bool format_value(Output& output, const FormatFlags& flags, int32_t value)
    {
        return format_value(output, flags, int64_t(value));
    }

    bool format_value(Output& output, const FormatFlags& flags, const char str[])
    {
        // determine the amount of characters to write
        auto nchars = int32_t(strlen(str));

        if (flags.precision >= 0) {
            nchars = std::min(flags.precision, nchars);
        }

        // determine width
        const int32_t width = std::max(flags.width, nchars);

        // determine alignment
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;

        switch (flags.align) {
        case '^':
            leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
            tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
            leadSpace += (width - nchars - 1) & 1; // if both are even or both are odd, we need to add one for correction
            tailSpace -= (width - nchars - 1) & 1; // if both are even or both are odd, we need to remove one for correction
            break;
        case '>':
            leadSpace = width - nchars;
            break;
        case '<':
        default:
            tailSpace = width - nchars;
            break;
        }

        // apply leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            output.write(fill);
        }

        // write string
        output.write(nchars, str);

        // apply tailing padding
        while (tailSpace--) {
            output.write(fill);
        }

        return true;
    }

    struct DummyArg {
    };

    bool format_value(Output&, const FormatFlags&, const DummyArg&)
    {
        return false;
    }

    bool format_index(Output&, const FormatFlags&, int32_t)
    {
        return false;
    }

    template <class Arg, class... Rest>
    bool format_index(Output& output, const FormatFlags& flags, int32_t index, Arg&& arg, Rest&&... rest)
    {
        if (!index) {
            return format_value(output, flags, std::forward<Arg>(arg));
        } else {
            return format_index(output, flags, index - 1, std::forward<Rest>(rest)...);
        }
    }

    template <class... Args>
    int32_t format(Output& output, const char fmt[], Args&&... args)
    {
        enum State {
            STATE_OPENER,
            STATE_INDEX,
            STATE_FLAGS,
            STATE_ALIGN,
            STATE_SIGN,
            STATE_ALTERNATE,
            STATE_WIDTH,
            STATE_PRECISION,
            STATE_TYPE,
            STATE_CLOSER,
        };

        auto before = output.result();
        auto state = STATE_OPENER;
        auto next = fmt;
        auto start = fmt;

        FormatFlags flags;
        auto prev = -1;
        auto index = -1;

        while (*next) {
            const auto ptr = next++;
            const auto ch = *ptr;

            switch (state) {
            case STATE_OPENER:
                if (ch == '{') {
                    output.write(int32_t(ptr - start), start);
                    if (*next != '{') {
                        index = -1;
                        start = ptr;
                        flags = FormatFlags();
                        state = STATE_INDEX;
                    } else {
                        start = next++;
                    }
                } else if (ch == '}') {
                    output.write(int32_t(next - start), start);
                    if (*next == '}') {
                        ++next;
                    }
                    start = next;
                }
                break;

            case STATE_INDEX:
                if (ch >= '0' && ch <= '9') {
                    index = (index < 0)
                        ? (ch - '0')
                        : (index * 10) + (ch - '0');
                } else {
                    if (index < 0) {
                        index = prev + 1;
                    }
                    prev = index;
                    state = STATE_FLAGS;
                    --next;
                }
                break;

            case STATE_FLAGS:
                if (ch == ':') {
                    state = STATE_ALIGN;
                } else {
                    state = STATE_CLOSER;
                    --next;
                }
                break;

            case STATE_ALIGN:
                if (ch == '}' || ch == '{') {
                    --next;
                } else {
                    switch (*next) {
                    case '<':
                    case '>':
                    case '=':
                    case '^':
                        flags.fill = ch;
                        flags.align = *next++;
                        state = STATE_SIGN;
                        break;
                    default:
                        switch (ch) {
                        case '<':
                        case '>':
                        case '=':
                        case '^':
                            flags.align = ch;
                            break;
                        default:
                            --next;
                            break;
                        }
                    }
                }
                state = STATE_SIGN;
                break;

            case STATE_SIGN:
                switch (ch) {
                case '+':
                case '-':
                case ' ':
                    flags.sign = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_ALTERNATE;
                break;

            case STATE_ALTERNATE:
                if (ch == '#') {
                    flags.alternate = true;
                } else {
                    --next;
                }
                state = STATE_WIDTH;
                break;

            case STATE_WIDTH:
                if (ch >= '0' && ch <= '9') {
                    if (flags.width < 0) {
                        if (ch == '0') {
                            flags.fill = '0';
                            flags.align = '=';
                        }
                        flags.width = 0;
                    }
                    flags.width = (flags.width * 10) + (ch - '0');
                } else {
                    state = STATE_PRECISION;
                    --next;
                }
                break;

            case STATE_PRECISION:
                if (flags.precision < 0 && ch == '.') {
                    flags.precision = 0;
                } else if (flags.precision >= 0 && ch >= '0' && ch <= '9') {
                    flags.precision = (flags.precision * 10) + (ch - '0');
                } else {
                    state = STATE_TYPE;
                    --next;
                }
                break;

            case STATE_TYPE:
                switch (ch) {
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'n':
                case 'o':
                case 's':
                case 'x':
                case 'X':
                case '%':
                    flags.type = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_CLOSER;
                break;

            case STATE_CLOSER:
                // if we get here without a valid closer, or an escaped opener,
                // the format is bogus and we'll ignore it.
                if (ch == '}' && format_index(output, flags, index, std::forward<Args>(args)...)) {
                    start = next;
                }
                state = STATE_OPENER;
                break;
            }
        }

        // Print remaining data
        if (start != next) {
            output.write(int32_t(next - start), start);
        }

        auto after = output.result();
        return after < 0 ? after : (after - before);
    }

    template <class... Args>
    int32_t print(const char fmt[], Args&&... args)
    {
        Output output;
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <class... Args>
    int32_t format(FILE* file, const char fmt[], Args&&... args)
    {
        Output output(file);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <class... Args>
    int32_t format(char buffer[], size_t size, const char fmt[], Args&&... args)
    {
        Output output(buffer, size);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const char fmt[], Args&&... args)
    {
        Output output(buffer, N);
        format(output, fmt, std::forward<Args>(args)...);
        return output.result();
    }

} // namespace sp
