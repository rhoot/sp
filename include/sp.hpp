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

#include <cmath> // NAN, INFINITY
#include <cstddef> // std::nullptr_t
#include <cstdint> // int32_t, uint64_t
#include <cstdio> // std::snprintf, std::FILE, std::fwrite
#include <cstring> // std::memcpy
#include <cctype> // std::isupper
#include <algorithm> // std::min, std::max
#include <limits> // std::numeric_limits
#include <utility> // std::forward

///
// API
///

namespace sp {

    /// Writer interface for the formatters.
    struct IWriter {
        /// Write the provided data to the output.
        virtual size_t write(size_t length, const void* data) = 0;
    };

    /// View into a string.
    struct StringView {
        const char* ptr = nullptr; //< Pointer to the string.
        int32_t length = 0; //< Length of the string.

        /// Construct an empty StringView.
        StringView();

        /// Construct a StringView from the provided null-terminated string.
        StringView(const char str[]);

        /// Construct a StringView from the provided string, with the provided
        /// length (in `char`).
        StringView(const char str[], int32_t length);
    };

    /// Print to standard out using the provided format with the provided
    /// format arguments. Return the amount of `char`s written, or `-1` in case
    /// of an error.
    template <class... Args>
    int32_t print(const StringView& fmt, Args&&... args);

    /// Print to the provided writer using the provided format with the
    /// provided format arguments.
    template <class... Args>
    void format(IWriter& writer, const StringView& fmt, Args&&... args);

    /// Print to the provided FILE stream using the provided format with the
    /// provided format arguments. Return the amount of `char`s written, or
    /// `-1` in case of an error.
    template <class... Args>
    int32_t format(std::FILE* file, const StringView& fmt, Args&&... args);

    /// Print to the provided buffer of the provided size, using the provided
    /// format string with the provided format arguments. Return the amount of
    /// `char`s that make up the resulting formatted string. If the buffer was
    /// not big enough to hold the entire result, the returned value may be
    /// larger than the buffer size. Return `-1` in case of an error.
    template <class... Args>
    int32_t format(char buffer[], size_t size, const StringView& fmt, Args&&... args);

    /// Print to the provided statically sized buffer, using the provided
    /// format string with the provided format arguments. Return the amount of
    /// `char`s that make up the resulting formatted string. If the buffer was
    /// not big enough to hold the entire result, the returned value may be
    /// larger than the buffer size. Return `-1` in case of an error.
    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const StringView& fmt, Args&&... args);

    /// Provided format functions.
    bool format_value(IWriter& writer, const StringView& fmt, std::nullptr_t);
    bool format_value(IWriter& writer, const StringView& fmt, bool value);
    bool format_value(IWriter& writer, const StringView& fmt, float value);
    bool format_value(IWriter& writer, const StringView& fmt, double value);
    bool format_value(IWriter& writer, const StringView& fmt, char value);
    bool format_value(IWriter& writer, const StringView& fmt, char16_t value);
    bool format_value(IWriter& writer, const StringView& fmt, char32_t value);
    bool format_value(IWriter& writer, const StringView& fmt, wchar_t value);
    bool format_value(IWriter& writer, const StringView& fmt, signed char value);
    bool format_value(IWriter& writer, const StringView& fmt, unsigned char value);
    bool format_value(IWriter& writer, const StringView& fmt, short value);
    bool format_value(IWriter& writer, const StringView& fmt, unsigned short value);
    bool format_value(IWriter& writer, const StringView& fmt, int value);
    bool format_value(IWriter& writer, const StringView& fmt, unsigned value);
    bool format_value(IWriter& writer, const StringView& fmt, long value);
    bool format_value(IWriter& writer, const StringView& fmt, unsigned long value);
    bool format_value(IWriter& writer, const StringView& fmt, long long value);
    bool format_value(IWriter& writer, const StringView& fmt, unsigned long long value);
    bool format_value(IWriter& writer, const StringView& fmt, char value[]);
    bool format_value(IWriter& writer, const StringView& fmt, const char value[]);
    bool format_value(IWriter& writer, const StringView& fmt, const StringView& value);

    template <class T>
    bool format_value(IWriter& output, const StringView& fmt, T* value);

} // namespace sp

///
// Implementation
///

namespace sp {

    class StringWriter : public IWriter {
    public:
        StringWriter(char buffer[], size_t size)
            : m_buffer(buffer)
            , m_size(int32_t(size))
            , m_length(0)
        {
        }

        int32_t result() const
        {
            return m_length;
        }

        size_t write(size_t length, const void* data) override
        {
            if (m_length >= 0) {
                m_length += int32_t(length);

                if (m_size) {
                    const auto toCopy = std::min(size_t(m_size), length);
                    std::memcpy(m_buffer, data, toCopy);
                    m_buffer += toCopy;
                    m_size -= int32_t(toCopy);
                    return toCopy;
                }
            }

            return 0;
        }

    private:
        char* m_buffer;
        int32_t m_size;
        int32_t m_length;
    };

    class StreamWriter : public IWriter {
    public:
        StreamWriter(FILE* stream)
            : m_stream(stream)
            , m_length(0)
        {
        }

        int32_t result() const
        {
            return m_length;
        }

        size_t write(size_t length, const void* data) override
        {
            if (m_length >= 0) {
                const auto written = std::fwrite(data, 1, length, m_stream);

                if (written == length) {
                    m_length += int32_t(written);
                } else {
                    m_length = -1;
                }

                return written;
            }

            return 0;
        }

    private:
        FILE* m_stream;
        int32_t m_length;
    };

    inline void write_char(IWriter& writer, char ch)
    {
        writer.write(1, &ch);
    }

    inline StringView::StringView() {}

    inline StringView::StringView(const char str[])
        : ptr(str)
        , length(str ? int32_t(std::strlen(str)) : 0)
    {
    }

    inline StringView::StringView(const char str[], int32_t length)
        : ptr(str)
        , length(length)
    {
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

    inline bool parse_format(const StringView& fmt, FormatFlags* flags)
    {
        enum State {
            STATE_ALIGN,
            STATE_SIGN,
            STATE_ALTERNATE,
            STATE_WIDTH,
            STATE_PRECISION,
            STATE_TYPE,
            STATE_DONE,
        };

        auto state = STATE_ALIGN;
        auto next = fmt.ptr;
        auto term = fmt.ptr + fmt.length;
        *flags = FormatFlags{};

        while (next < term) {
            const auto ptr = next++;
            const auto ch = *ptr;

            switch (state) {
            case STATE_ALIGN: {
                const auto isAlign = [](char ch) {
                    return (ch >= '<' && ch <= '>') || ch == '^';
                };

                if (next < term && isAlign(*next)) {
                    flags->fill = ch;
                    flags->align = *next++;
                } else if (isAlign(ch)) {
                    flags->align = ch;
                } else {
                    --next;
                }

                state = STATE_SIGN;
                break;
            }

            case STATE_SIGN:
                switch (ch) {
                case '+':
                case '-':
                case ' ':
                    flags->sign = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_ALTERNATE;
                break;

            case STATE_ALTERNATE:
                if (ch == '#') {
                    flags->alternate = true;
                } else {
                    --next;
                }
                state = STATE_WIDTH;
                break;

            case STATE_WIDTH:
                if (ch >= '0' && ch <= '9') {
                    if (flags->width < 0) {
                        if (ch == '0') {
                            flags->fill = flags->fill ? flags->fill : '0';
                            flags->align = flags->align ? flags->align : '=';
                        }
                        flags->width = 0;
                    }
                    flags->width = (flags->width * 10) + (ch - '0');
                } else {
                    state = STATE_PRECISION;
                    --next;
                }
                break;

            case STATE_PRECISION: {
                const auto isDigit = [](char ch) {
                    return ch >= '0' && ch <= '9';
                };

                if (ch == '.') {
                    const char nextCh = (next < term) ? *next : 0;

                    if (flags->precision < 0 && isDigit(nextCh)) {
                        flags->precision = 0;
                        continue;
                    }
                } else if (isDigit(ch)) {
                    if (flags->precision >= 0) {
                        flags->precision = (flags->precision * 10) + (ch - '0');
                        continue;
                    }
                }

                state = STATE_TYPE;
                --next;
                break;
            }

            case STATE_TYPE:
                switch (ch) {
                case 'b':
                case 'd':
                case 'c':
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'o':
                case 's':
                case 'x':
                case 'X':
                case '%':
                    flags->type = ch;
                    break;
                default:
                    --next;
                    break;
                }
                state = STATE_DONE;
                break;

            case STATE_DONE:
                // if we get here, it means there's stuff in there after the
                // flags, in which case we should fail.
                return false;
            }
        }

        return true;
    }

    inline bool format_int(IWriter& writer, const FormatFlags& flags, bool isNegative, uint64_t value)
    {
        // determine base
        int32_t base = 10;

        switch (flags.type) {
        case 'b':
            base = 2;
            break;
        case 'o':
            base = 8;
            break;
        case 'c':
        case 'x':
        case 'X':
            base = 16;
            break;
        }

        // count digits, and copy them to a buffer (so we don't have to repeat
        // this later)
        char buffer[67]; // max needed; 64-bit binary + sign + alternate prefix
        char* digits = buffer + sizeof(buffer);
        char* prefix = buffer + 3;
        int32_t ndigits = 0;
        int32_t nprefix = 0;
        char type = flags.type;

        const bool isChar = type == 'c';
        const bool charAsHex = (value >= 0x80 || isNegative);

        if (isChar) {
            if (charAsHex) {
                *(--digits) = ')';
            } else {
                *(--digits) = char(value);
            }
            ndigits = 1;
        }

        if (!isChar || charAsHex) {
            const auto digitchars = (flags.type == 'X')
                ? "0123456789ABCDEFX"
                : "0123456789abcdefx";

            uint64_t v = value;
            do {
                *(--digits) = digitchars[v % base];
                v /= base;
                ++ndigits;
            } while (v);

            if (flags.alternate) {
                switch (base) {
                case 2:
                    *(--prefix) = 'b';
                    *(--prefix) = '0';
                    nprefix += 2;
                    break;
                case 8:
                    *(--prefix) = 'o';
                    *(--prefix) = '0';
                    nprefix += 2;
                    break;
                case 16:
                    *(--prefix) = digitchars[16];
                    *(--prefix) = '0';
                    nprefix += 2;
                    break;
                }
            }
        }

        // determine sign
        if (isNegative) {
            *(--prefix) = '-';
            ++nprefix;
        } else if (flags.sign == '+' || flags.sign == ' ') {
            *(--prefix) = flags.sign;
            ++nprefix;
        }

        // for hex chars, the prefix should be part of the value
        if (isChar && charAsHex) {
            if (nprefix) {
                digits -= nprefix;
                ndigits += nprefix;
                memcpy(digits, prefix, nprefix);
                nprefix = 0;
            }
            *(--digits) = '(';
            ++ndigits;
        }

        // determine spacing
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;
        {
            int32_t nchars = ndigits + nprefix;
            int32_t width = std::max(flags.width, nchars);

            switch (flags.align) {
            case '^':
                leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
                tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
                leadSpace += (width & 1) & (nchars & 1); // if both are odd, we need to add one for correction
                tailSpace -= (width & 1) & (nchars & 1); // if both are odd, we need to remove one for correction
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

        // print sign and alternate prefix, if they should be before the padding
        if (nprefix) {
            switch (flags.align) {
            case '^':
            case '>':
                break;
            case '<':
            case '=':
            default:
                writer.write(nprefix, prefix);
                nprefix = 0;
                break;
            }
        }

        // apply the leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            write_char(writer, fill);
        }

        // print the prefix, if it should be after the padding
        if (nprefix) {
            writer.write(nprefix, prefix);
        }

        // print digits
        writer.write(ndigits, digits);

        // print tailing padding
        while (tailSpace--) {
            write_char(writer, fill);
        }

        return true;
    }

    template <class F>
    bool format_float(IWriter& writer, const StringView& format, F value)
    {
        FormatFlags flags;

        if (!parse_format(format, &flags)) {
            return false;
        }

        // I *really* have no interest in serializing floats/doubles... so
        // let's not. Instead, let's build a format string for snprintf to do
        // the heavy work, and we'll just do alignment and stuff.
        const char* suffix = "";
        int32_t precision;
        char type;

        switch (flags.type) {
        case 'f':
        case 'F':
        case 'e':
        case 'E':
            precision = flags.precision >= 0 ? flags.precision : 6;
            type = flags.type;
            break;
        case 'g':
        case 'G':
            precision = (flags.precision != 0)
                ? (flags.precision > 0 ? flags.precision : 6)
                : 1;
            type = flags.type;
            break;
        case '%':
            precision = flags.precision >= 0 ? flags.precision : 6;
            type = 'f';
            value *= 100;
            suffix = "%%";
            break;
        default:
            precision = flags.precision >= 0 ? flags.precision : std::numeric_limits<F>::digits10;
            type = 'g';
            break;
        }

        // produce the formatted value
        char buffer[512];
        const char* digits = buffer;
        int32_t ndigits = 0;

        if (std::isnan(value)) {
            const char* str = std::isupper(flags.type) ? "NAN" : "nan";
            strncpy(buffer, str, 3);
            ndigits = 3;
        } else if (std::isinf(value)) {
            const char* str = std::isupper(flags.type) ? "INF" : "inf";
            strncpy(buffer, str, 3);
            ndigits = 3;
        } else {
            char numFormat[16];
            snprintf(numFormat, sizeof(numFormat), "%%+.%d%c%s", precision, type, suffix);

            // produce the formatted value
    #if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900)
            unsigned int prevOutputFormat = _set_output_format(_TWO_DIGIT_EXPONENT);
    #endif

            ndigits = snprintf(buffer, sizeof(buffer), numFormat, value) - 1;
            digits = buffer + 1;

    #if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER < 1900)
            _set_output_format(prevOutputFormat);
    #endif
        }

        // determine sign
        char sign = 0;

        if (value < 0) {
            sign = '-';
        } else if (flags.sign == '+' || flags.sign == ' ') {
            sign = flags.sign;
        }

        // determine width
        const int nchars = sign ? ndigits + 1 : ndigits;
        const int32_t width = std::max(flags.width, nchars);

        // determine alignment
        int32_t leadSpace = 0;
        int32_t tailSpace = 0;

        switch (flags.align) {
        case '^':
            leadSpace = (width / 2) - ((nchars + 1) / 2); // nchars rounded up
            tailSpace = ((width + 1) / 2) - (nchars / 2); // width rounded up
            leadSpace += (width & 1) & (nchars & 1); // if both are odd, we need to add one for correction
            tailSpace -= (width & 1) & (nchars & 1); // if both are odd, we need to remove one for correction
            break;
        case '<':
            tailSpace = width - nchars;
            break;
        case '>':
        default:
            leadSpace = width - nchars;
            break;
        }

        // print sign, if it should be before the padding
        if (sign && flags.align == '=') {
            write_char(writer, sign);
        }

        // apply leading padding
        const char fill = flags.fill ? flags.fill : ' ';

        while (leadSpace--) {
            write_char(writer, fill);
        }

        // print sign, if it should be after the padding
        if (sign && flags.align != '=') {
            write_char(writer, sign);
        }

        // write string
        writer.write(ndigits, digits);

        // apply tailing padding
        while (tailSpace--) {
            write_char(writer, fill);
        }

        return true;
    }

    inline bool format_string(IWriter& writer, const sp::FormatFlags& flags, const StringView& str)
    {
        // determine the amount of characters to write
        auto nchars = str.length;

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
            leadSpace += (width & 1) & (nchars & 1); // if both are odd, we need to add one for correction
            tailSpace -= (width & 1) & (nchars & 1); // if both are odd, we need to remove one for correction
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
            write_char(writer, fill);
        }

        // write string
        writer.write(nchars, str.ptr);

        // apply tailing padding
        while (tailSpace--) {
            write_char(writer, fill);
        }

        return true;
    }

    struct DummyArg {
    };

    inline bool format_value(IWriter&, const StringView&, const DummyArg&)
    {
        return false;
    }

    inline bool format_index(IWriter&, const StringView&, int32_t)
    {
        return false;
    }

    template <class Arg, class... Rest>
    bool format_index(IWriter& writer, const StringView& format, int32_t index, Arg&& arg, Rest&&... rest)
    {
        if (!index) {
            return format_value(writer, format, std::forward<Arg>(arg));
        } else {
            return format_index(writer, format, index - 1, std::forward<Rest>(rest)...);
        }
    }

    template <class... Args>
    int32_t print(const StringView& fmt, Args&&... args)
    {
        StreamWriter writer(stdout);
        format(writer, fmt, std::forward<Args>(args)...);
        return writer.result();
    }

    template <class... Args>
    void do_format(IWriter& writer, const StringView& fmt, int32_t* prevIndex, Args&&... args)
    {
        enum State {
            STATE_OPENER,
            STATE_INDEX,
            STATE_MARKER,
            STATE_FLAGS,
            STATE_CLOSER,
        };

        auto state = STATE_OPENER;
        auto next = fmt.ptr;
        auto start = fmt.ptr;
        auto term = fmt.ptr + fmt.length;

        const char* formatStart = nullptr;
        auto index = -1;
        auto opened = 0;
        auto nested = false;

        while (next < term) {
            const auto ptr = next++;
            const auto ch = *ptr;

            switch (state) {
            case STATE_OPENER:
                if (ch == '{') {
                    writer.write(int32_t(ptr - start), start);

                    if (next < term) {
                        if (*next != '{') {
                            index = -1;
                            start = ptr;
                            nested = false;
                            formatStart = nullptr;
                            state = STATE_INDEX;
                        } else {
                            start = next++;
                        }
                    }
                } else if (ch == '}') {
                    writer.write(int32_t(next - start), start);
                    if (next < term && *next == '}') {
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
                        index = *prevIndex + 1;
                    }
                    *prevIndex = index;
                    state = STATE_MARKER;
                    --next;
                }
                break;

            case STATE_MARKER:
                if (ch == ':') {
                    state = STATE_FLAGS;
                } else {
                    state = STATE_CLOSER;
                    --next;
                }
                break;

            case STATE_FLAGS:
                if (!formatStart) {
                    formatStart = ptr;
                }

                switch (ch) {
                case '{':
                    ++opened;
                    nested = true;
                    break;
                case '}':
                    if (opened > 0) {
                        --opened;
                    }
                    else {
                        state = STATE_CLOSER;
                        --next;
                    }
                    break;
                }

                break;

            case STATE_CLOSER:
                if (ch == '}') {
                    StringView format = formatStart
                        ? StringView(formatStart, int32_t(ptr - formatStart))
                        : StringView();

                    // handle nested format specifiers
                    char buffer[64];

                    if (nested) {
                        StringWriter nestedWriter(buffer, sizeof(buffer));
                        sp::do_format(nestedWriter, format, prevIndex, std::forward<Args>(args)...);
                        const auto fullLen = nestedWriter.result();
                        const auto realLen = std::min(size_t(fullLen), sizeof(buffer));
                        format = StringView(buffer, int32_t(realLen));
                    }

                    if (format_index(writer, format, index, std::forward<Args>(args)...)) {
                        start = next;
                    }
                }
                state = STATE_OPENER;
                break;
            }
        }

        // Print remaining data
        if (start != term) {
            writer.write(int32_t(term - start), start);
        }
    }

    template <class... Args>
    void format(IWriter& writer, const StringView& fmt, Args&&... args)
    {
        int32_t prevIndex = -1;
        do_format(writer, fmt, &prevIndex, std::forward<Args>(args)...);
    }

    template <class... Args>
    int32_t format(std::FILE* file, const StringView& fmt, Args&&... args)
    {
        StreamWriter writer(file);
        format(writer, fmt, std::forward<Args>(args)...);
        return writer.result();
    }

    template <class... Args>
    int32_t format(char buffer[], size_t size, const StringView& fmt, Args&&... args)
    {
        StringWriter writer(buffer, size);
        format(writer, fmt, std::forward<Args>(args)...);
        return writer.result();
    }

    template <size_t N, class... Args>
    int32_t format(char (&buffer)[N], const StringView& fmt, Args&&... args)
    {
        StringWriter writer(buffer, N);
        format(writer, fmt, std::forward<Args>(args)...);
        return writer.result();
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, std::nullptr_t)
    {
        return format_value(writer, fmt, (void*)0);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, bool value)
    {
        FormatFlags flags;

        if (parse_format(fmt, &flags)) {
            switch (flags.type) {
                case 'b':
                case 'c':
                case 'd':
                case 'o':
                case 'x':
                case 'X':
                    return format_int(writer, flags, false, (uint64_t)value);
                default:
                    return format_string(writer, flags, value ? "true" : "false");
            }
        }

        return false;
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, float value)
    {
        return format_float(writer, fmt, value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, double value)
    {
        return format_float(writer, fmt, value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, char value)
    {
        return format_value(writer, fmt, char32_t(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, char16_t value)
    {
        return format_value(writer, fmt, char32_t(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, char32_t value)
    {
        FormatFlags flags;

        if (parse_format(fmt, &flags)) {
            if (!flags.type) {
                flags.type = 'c';
            }

            if (!flags.align) {
                flags.align = '<';
            }

            return format_int(writer, flags, false, uint64_t(value));
        }

        return false;
    }

    template <size_t S> struct WcharSelector;
    template<> struct WcharSelector<2> { using Type = char16_t; };
    template<> struct WcharSelector<4> { using Type = char32_t; };

    inline bool format_value(IWriter& writer, const StringView& fmt, wchar_t value)
    {
        using CharType = typename WcharSelector<sizeof(wchar_t)>::Type;
        return format_value(writer, fmt, CharType(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, signed char value)
    {
        return format_value(writer, fmt, (long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, unsigned char value)
    {
        return format_value(writer, fmt, (unsigned long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, short value)
    {
        return format_value(writer, fmt, (long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, unsigned short value)
    {
        return format_value(writer, fmt, (unsigned long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, int value)
    {
        return format_value(writer, fmt, (long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, unsigned value)
    {
        return format_value(writer, fmt, (unsigned long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, long value)
    {
        return format_value(writer, fmt, (long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, unsigned long value)
    {
        return format_value(writer, fmt, (unsigned long long)value);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, long long value)
    {
        static_assert(sizeof(value) == sizeof(uint64_t), "invalid cast on negation");

        const auto abs = (value >= 0 || value == std::numeric_limits<long long>::min())
            ? uint64_t(value)
            : uint64_t(-value);

        FormatFlags flags;

        return parse_format(fmt, &flags)
            && format_int(writer, flags, value < 0, abs);
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, unsigned long long value)
    {
        FormatFlags flags;

        return parse_format(fmt, &flags)
            && format_int(writer, flags, false, uint64_t(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, char value[])
    {
        return format_value(writer, fmt, StringView(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, const char value[])
    {
        return format_value(writer, fmt, StringView(value));
    }

    inline bool format_value(IWriter& writer, const StringView& fmt, const StringView& value)
    {
        FormatFlags flags;

        return parse_format(fmt, &flags)
            ? format_string(writer, flags, value)
            : false;
    }

    template <class T>
    bool format_value(IWriter& writer, const StringView& fmt, T* value)
    {
        FormatFlags flags;

        if (parse_format(fmt, &flags)) {
            if (!flags.type) {
                flags.type = 'x';
            }

            return format_int(writer, flags, false, uint64_t(value));
        }

        return false;
    }

} // namespace sp
