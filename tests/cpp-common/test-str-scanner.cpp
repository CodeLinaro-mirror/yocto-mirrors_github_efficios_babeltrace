/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstring>
#include <limits>
#include <optional>
#include <string_view>

#include "cpp-common/bt2c/exc.hpp"
#include "cpp-common/bt2c/logging.hpp"
#include "cpp-common/bt2c/regex.hpp"
#include "cpp-common/bt2c/str-scanner.hpp"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

namespace {

/*
 * Concrete `bt2c::StrScanner` subclass which exposes its protected
 * members publicly (so that this file may test them) and which skips
 * whitespace characters as noise, like a typical concrete subclass
 * would do.
 */
class TestStrScanner final : public bt2c::StrScanner
{
public:
    explicit TestStrScanner(const std::string_view str)
        : bt2c::StrScanner {str, _logger()}
    {
    }

    explicit TestStrScanner(const std::string_view str, const std::size_t baseOffset)
        : bt2c::StrScanner {str, baseOffset, _logger()}
    {
    }

    unsigned int skipNoiseCallCount() const noexcept
    {
        return _mSkipNoiseCallCount;
    }

    int tryScanAnyChar() noexcept
    {
        return this->_tryScanAnyChar();
    }

    void incrAt(const std::size_t count = 1) noexcept
    {
        this->_incrAt(count);
    }

    void incrAtWithNewlineCheck() noexcept
    {
        this->_incrAtWithNewlineCheck();
    }

    void decrAt(const std::size_t count = 1) noexcept
    {
        this->_decrAt(count);
    }

    std::optional<std::string_view> tryScanLitStr(const std::string_view escapeSeqStartList)
    {
        return this->_tryScanLitStr(escapeSeqStartList);
    }

    std::string_view tryScanUnquotedStr(const std::string_view extraFirstChars = {},
                                        const std::string_view extraOtherChars = {}) noexcept
    {
        return this->_tryScanUnquotedStr(extraFirstChars, extraOtherChars);
    }

    void skipWhitespaces() noexcept
    {
        this->_skipWhitespaces();
    }

    std::optional<double> tryScanConstReal(const bt2c::Regex& regex) noexcept
    {
        return this->_tryScanConstReal(regex);
    }

private:
    static const bt2c::Logger& _logger()
    {
        static const bt2c::Logger logger {"test-module", "test-tag", bt2c::Logger::Level::None};

        return logger;
    }

    void _skipNoise() noexcept override
    {
        ++_mSkipNoiseCallCount;
        this->_skipWhitespaces();
    }

    unsigned int _mSkipNoiseCallCount = 0;
};

/*
 * Minimal `bt2c::StrScanner` subclass which doesn't override
 * `_skipNoise()`, to test the default (no-op) implementation.
 */
class DefaultNoiseStrScanner final : public bt2c::StrScanner
{
public:
    explicit DefaultNoiseStrScanner(const std::string_view str)
        : bt2c::StrScanner {str,
                            bt2c::Logger {"test-module", "test-tag", bt2c::Logger::Level::None}}
    {
    }
};

template <typename FuncT>
void requireThrowsAndClearError(FuncT&& func, const std::string_view expectedMsgSubstr)
{
    REQUIRE_THROWS_AS(func(), bt2c::Error);

    const auto error = bt_current_thread_take_error();

    REQUIRE(error);
    CHECK_THAT(bt_error_cause_get_message(bt_error_borrow_cause_by_index(error, 0)),
               Catch::Matchers::ContainsSubstring(std::string {expectedMsgSubstr}));
    bt_error_release(error);
}

} /* namespace */

TEST_CASE("bt2c::StrScanner::at() returns str().begin() initially")
{
    const TestStrScanner ss {"abc"};

    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::at() sets the current position")
{
    TestStrScanner ss {"abc"};

    ss.at(ss.str().begin() + 2);
    CHECK(ss.at() == ss.str().begin() + 2);
}

TEST_CASE("bt2c::StrScanner::str() returns the wrapped string")
{
    const TestStrScanner ss {"hello"};

    CHECK(ss.str() == "hello");
}

TEST_CASE("bt2c::StrScanner::charsLeft() returns the number of characters left to scan")
{
    TestStrScanner ss {"hello"};

    CHECK(ss.charsLeft() == 5);
    ss.incrAt(2);
    CHECK(ss.charsLeft() == 3);
    ss.incrAt(3);
    CHECK(ss.charsLeft() == 0);
}

TEST_CASE("bt2c::StrScanner::loc() returns offset 0, line 0, and column 0 initially")
{
    const TestStrScanner ss {"ab\ncd\nef"};
    const auto loc = ss.loc();

    CHECK(loc.offset() == 0);
    CHECK(loc.lineNo() == 0);
    CHECK(loc.colNo() == 0);
}

TEST_CASE(
    "bt2c::StrScanner::loc() tracks the offset, line number, and column number while advancing")
{
    TestStrScanner ss {"ab\ncd\nef"};

    /* Consume `ab\ncd\n`, landing right before `ef` */
    for (auto i = 0U; i < 6; ++i) {
        ss.incrAtWithNewlineCheck();
    }

    const auto loc = ss.loc();

    CHECK(loc.offset() == 6);
    CHECK(loc.lineNo() == 2);
    CHECK(loc.colNo() == 0);
    CHECK(loc.naturalLineNo() == 3);
    CHECK(loc.naturalColNo() == 1);
}

TEST_CASE("bt2c::StrScanner::loc() considers the base offset passed to the constructor")
{
    CHECK(TestStrScanner {"abc", 100}.loc().offset() == 100);
}

TEST_CASE(
    "bt2c::StrScanner::isDone() returns false before the end of the wrapped string is reached")
{
    CHECK_FALSE(TestStrScanner {"a"}.isDone());
}

TEST_CASE("bt2c::StrScanner::isDone() returns true at the end of the wrapped string")
{
    TestStrScanner ss {"a"};

    ss.incrAt();
    CHECK(ss.isDone());
}

TEST_CASE("bt2c::StrScanner::curChar() returns the character at the current position")
{
    TestStrScanner ss {"ab"};

    CHECK(ss.curChar() == 'a');
    ss.incrAt();
    CHECK(ss.curChar() == 'b');
}

TEST_CASE("bt2c::StrScanner::skipNoise() calls the overridden _skipNoise() method")
{
    TestStrScanner ss {"   abc"};

    ss.skipNoise();
    CHECK(ss.skipNoiseCallCount() == 1);
    CHECK(ss.at() == ss.str().begin() + 3);
}

TEST_CASE("bt2c::StrScanner::reset() resets the current position, line info, and position stack")
{
    TestStrScanner ss {"ab\ncd"};

    ss.incrAtWithNewlineCheck();
    ss.incrAtWithNewlineCheck();
    ss.incrAtWithNewlineCheck();
    ss.save();
    REQUIRE(ss.at() != ss.str().begin());
    REQUIRE(ss.loc().lineNo() == 1);

    ss.reset();
    CHECK(ss.at() == ss.str().begin());
    CHECK(ss.loc().offset() == 0);
    CHECK(ss.loc().lineNo() == 0);
    CHECK(ss.loc().colNo() == 0);

    /* Position stack was also cleared: this doesn't assert */
    ss.save();
    ss.accept();
}

TEST_CASE("bt2c::StrScanner::save() and bt2c::StrScanner::accept() keep the current position")
{
    TestStrScanner ss {"abc"};

    ss.incrAt();
    ss.save();
    ss.incrAt();
    ss.accept();
    CHECK(ss.at() == ss.str().begin() + 2);
}

TEST_CASE("bt2c::StrScanner::save() and bt2c::StrScanner::reject() restore the saved position")
{
    TestStrScanner ss {"ab\ncd"};

    ss.incrAtWithNewlineCheck();

    const auto savedAt = ss.at();
    const auto savedLoc = ss.loc();

    ss.save();
    ss.incrAtWithNewlineCheck();
    ss.incrAtWithNewlineCheck();
    REQUIRE(ss.at() != savedAt);
    ss.reject();
    CHECK(ss.at() == savedAt);
    CHECK(ss.loc().offset() == savedLoc.offset());
    CHECK(ss.loc().lineNo() == savedLoc.lineNo());
    CHECK(ss.loc().colNo() == savedLoc.colNo());
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() scans a decimal unsigned integer")
{
    TestStrScanner ss {"1234 rest"};
    const auto val = ss.tryScanConstUInt();

    REQUIRE(val);
    CHECK(*val == 1234);
    CHECK(ss.at() == ss.str().begin() + 4);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() doesn't allow a negative sign")
{
    TestStrScanner ss {"-1234"};

    CHECK_FALSE(ss.tryScanConstUInt());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() returns `std::nullopt` for non-integer input")
{
    TestStrScanner ss {"nope"};

    CHECK_FALSE(ss.tryScanConstUInt());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() skips noise (whitespace) before scanning")
{
    TestStrScanner ss {"   42"};
    const auto val = ss.tryScanConstUInt();

    REQUIRE(val);
    CHECK(*val == 42);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() scans the minimum value")
{
    TestStrScanner ss {"0"};
    const auto val = ss.tryScanConstUInt();

    REQUIRE(val);
    CHECK(*val == 0);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() scans the maximum value")
{
    TestStrScanner ss {"18446744073709551615"};
    const auto val = ss.tryScanConstUInt();

    REQUIRE(val);
    CHECK(*val == std::numeric_limits<unsigned long long>::max());
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() returns `std::nullopt` on overflow")
{
    /* One more than the maximum value */
    TestStrScanner ss {"18446744073709551616"};

    CHECK_FALSE(ss.tryScanConstUInt());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() scans a negative decimal integer")
{
    TestStrScanner ss {"-1234"};
    const auto val = ss.tryScanConstSInt();

    REQUIRE(val);
    CHECK(*val == -1234);
    CHECK(ss.isDone());
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() scans a positive decimal integer")
{
    TestStrScanner ss {"1234"};
    const auto val = ss.tryScanConstSInt();

    REQUIRE(val);
    CHECK(*val == 1234);
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() scans the minimum value")
{
    TestStrScanner ss {"-9223372036854775808"};
    const auto val = ss.tryScanConstSInt();

    REQUIRE(val);
    CHECK(*val == std::numeric_limits<long long>::min());
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() scans the maximum value")
{
    TestStrScanner ss {"9223372036854775807"};
    const auto val = ss.tryScanConstSInt();

    REQUIRE(val);
    CHECK(*val == std::numeric_limits<long long>::max());
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() returns `std::nullopt` on positive overflow")
{
    /* One more than the maximum value */
    TestStrScanner ss {"9223372036854775808"};

    CHECK_FALSE(ss.tryScanConstSInt());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt() returns `std::nullopt` on negative overflow")
{
    /* One less than the minimum value */
    TestStrScanner ss {"-9223372036854775809"};

    CHECK_FALSE(ss.tryScanConstSInt());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE(
    "bt2c::StrScanner::tryScanConstUInt<true>() scans a hexadecimal integer with the `0x` prefix")
{
    TestStrScanner ss {"0x1f3a"};
    const auto val = ss.tryScanConstUInt<true>();

    REQUIRE(val);
    CHECK(*val == 0x1f3a);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt<true>() scans a binary integer with the `0b` prefix")
{
    TestStrScanner ss {"0b1101"};
    const auto val = ss.tryScanConstUInt<true>();

    REQUIRE(val);
    CHECK(*val == 0b1101);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt<true>() scans an octal integer with a leading `0`")
{
    TestStrScanner ss {"0777"};
    const auto val = ss.tryScanConstUInt<true>();

    REQUIRE(val);
    CHECK(*val == 0777);
}

TEST_CASE("bt2c::StrScanner::tryScanConstUInt() ignores radix prefixes without `AllowPrefixesV`")
{
    TestStrScanner ss {"0x1f3a"};
    const auto val = ss.tryScanConstUInt();

    REQUIRE(val);
    CHECK(*val == 0);
    CHECK(ss.at() == ss.str().begin() + 1);
}

TEST_CASE("bt2c::StrScanner::tryScanConstSInt<true>() scans a negative hexadecimal integer")
{
    TestStrScanner ss {"-0x1f"};
    const auto val = ss.tryScanConstSInt<true>();

    REQUIRE(val);
    CHECK(*val == -0x1f);
}

TEST_CASE("bt2c::StrScanner::tryScanToken() scans a matching token")
{
    TestStrScanner ss {"hello world"};

    CHECK(ss.tryScanToken("hello"));
    CHECK(ss.at() == ss.str().begin() + 5);
}

TEST_CASE("bt2c::StrScanner::tryScanToken() returns false for a non-matching token")
{
    TestStrScanner ss {"hello world"};

    CHECK_FALSE(ss.tryScanToken("goodbye"));
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE(
    "bt2c::StrScanner::tryScanToken() returns false when the wrapped string ends before the end of the token")
{
    TestStrScanner ss {"he"};

    CHECK_FALSE(ss.tryScanToken("hello"));
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::tryScanToken() skips noise (whitespace) before scanning")
{
    TestStrScanner ss {"   hello"};

    CHECK(ss.tryScanToken("hello"));
    CHECK(ss.isDone());
}

TEST_CASE("bt2c::StrScanner::_tryScanAnyChar() scans and advances over any single character")
{
    TestStrScanner ss {"ab"};

    CHECK(ss.tryScanAnyChar() == 'a');
    CHECK(ss.tryScanAnyChar() == 'b');
    CHECK(ss.tryScanAnyChar() == -1);
    CHECK(ss.isDone());
}

TEST_CASE("bt2c::StrScanner::_incrAt() advances the current position")
{
    TestStrScanner ss {"abcdef"};

    ss.incrAt();
    CHECK(ss.at() == ss.str().begin() + 1);
    ss.incrAt(3);
    CHECK(ss.at() == ss.str().begin() + 4);
}

TEST_CASE("bt2c::StrScanner::_incrAtWithNewlineCheck() advances the position and tracks newlines")
{
    TestStrScanner ss {"a\nb"};

    ss.incrAtWithNewlineCheck();
    CHECK(ss.loc().lineNo() == 0);
    CHECK(ss.loc().colNo() == 1);
    ss.incrAtWithNewlineCheck();
    CHECK(ss.loc().lineNo() == 1);
    CHECK(ss.loc().colNo() == 0);
    ss.incrAtWithNewlineCheck();
    CHECK(ss.loc().lineNo() == 1);
    CHECK(ss.loc().colNo() == 1);
}

TEST_CASE("bt2c::StrScanner::_decrAt() moves the current position backward")
{
    TestStrScanner ss {"abcdef"};

    ss.incrAt(4);
    ss.decrAt();
    CHECK(ss.at() == ss.str().begin() + 3);
    ss.decrAt(2);
    CHECK(ss.at() == ss.str().begin() + 1);
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() scans a simple literal string")
{
    TestStrScanner ss {R"("hello, world!" rest)"};
    const auto val = ss.tryScanLitStr({});

    REQUIRE(val);
    CHECK(*val == "hello, world!");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() returns `std::nullopt` and rewinds when there's no literal string")
{
    TestStrScanner ss {"hello"};

    CHECK_FALSE(ss.tryScanLitStr({}));
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() returns `std::nullopt` and rewinds when the closing double quote is missing")
{
    TestStrScanner ss {R"("hello)"};

    CHECK_FALSE(ss.tryScanLitStr({}));
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() always considers `\"` and `\\` as escape sequence starting characters")
{
    TestStrScanner ss {R"("a\"b\\c")"};
    const auto val = ss.tryScanLitStr({});

    REQUIRE(val);
    CHECK(*val == R"(a"b\c)");
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() decodes single-character escape sequences")
{
    TestStrScanner ss {R"("\a\b\f\n\r\t\v")"};
    const auto val = ss.tryScanLitStr("abfnrtv");

    REQUIRE(val);
    CHECK(*val == "\a\b\f\n\r\t\v");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() ignores single-character escape sequences not in `escapeSeqStartList`")
{
    /*
     * `\n` is a regular character because `escapeSeqStartList` doesn't
     * contain `n`.
     */
    TestStrScanner ss {R"("a\nb")"};
    const auto val = ss.tryScanLitStr("t");

    REQUIRE(val);
    CHECK(*val == "a\\nb");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() decodes a `\\u` Unicode escape sequence for a codepoint encoded on one UTF-8 byte")
{
    /* U+0041 (`A`) */
    TestStrScanner ss {R"("\u0041")"};
    const auto val = ss.tryScanLitStr("u");

    REQUIRE(val);
    CHECK(*val == "A");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() decodes a `\\u` Unicode escape sequence for a codepoint encoded on two UTF-8 bytes")
{
    /* U+03C9 (`ω`) */
    TestStrScanner ss {R"("\u03c9")"};
    const auto val = ss.tryScanLitStr("u");

    REQUIRE(val);
    CHECK(*val == "\xcf\x89");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() decodes a `\\u` Unicode escape sequence for a codepoint encoded on three UTF-8 bytes")
{
    /* U+4E2D (`中`) */
    TestStrScanner ss {R"("\u4e2d")"};
    const auto val = ss.tryScanLitStr("u");

    REQUIRE(val);
    CHECK(*val == "\xe4\xb8\xad");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a `\\u` escape sequence with not enough characters left")
{
    TestStrScanner ss {R"("\u12")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("u");
        },
        "needs four hexadecimal digits");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a `\\u` escape sequence with an invalid hexadecimal digit")
{
    TestStrScanner ss {R"("\u12zz")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("u");
        },
        "unexpected character `z`");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a `\\u` escape sequence with a surrogate codepoint")
{
    TestStrScanner ss {R"("\ud801")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("u");
        },
        "unsupported surrogate codepoint U+D801");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a `\\u` escape sequence with the lowest surrogate codepoint")
{
    /* U+D800 is the lowest codepoint of the surrogate range */
    TestStrScanner ss {R"("\ud800")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("u");
        },
        "unsupported surrogate codepoint U+D800");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a `\\u` escape sequence with the highest surrogate codepoint")
{
    /* U+DFFF is the highest codepoint of the surrogate range */
    TestStrScanner ss {R"("\udfff")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("u");
        },
        "unsupported surrogate codepoint U+DFFF");
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() decodes an octal escape sequence")
{
    TestStrScanner ss {R"("\101\102")"};
    const auto val = ss.tryScanLitStr("0");

    REQUIRE(val);
    CHECK(*val == "AB");
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() decodes a hexadecimal escape sequence")
{
    TestStrScanner ss {R"("\x41\x42")"};
    const auto val = ss.tryScanLitStr("x");

    REQUIRE(val);
    CHECK(*val == "AB");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a hexadecimal escape sequence with no hexadecimal digit")
{
    TestStrScanner ss {R"("\x")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("x");
        },
        "expecting at least one hexadecimal digit");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on a hexadecimal escape sequence value greater than 255")
{
    TestStrScanner ss {R"("\x100")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("x");
        },
        "value 0x100 is greater than 255");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() keeps an unrecognized escape sequence start character as is")
{
    TestStrScanner ss {R"("a\zb")"};
    const auto val = ss.tryScanLitStr({});

    REQUIRE(val);
    CHECK(*val == R"(a\zb)");
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() throws on an illegal control character")
{
    TestStrScanner ss {"\"a\tb\""};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr({});
        },
        "Illegal control character 0x9");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanLitStr() throws on an octal escape sequence value greater than 255")
{
    TestStrScanner ss {R"("\777")"};

    requireThrowsAndClearError(
        [&ss] {
            ss.tryScanLitStr("0");
        },
        "is greater than 255");
}

TEST_CASE("bt2c::StrScanner::_tryScanLitStr() skips noise (whitespace) before scanning")
{
    TestStrScanner ss {R"(   "hi")"};
    const auto val = ss.tryScanLitStr({});

    REQUIRE(val);
    CHECK(*val == "hi");
}

TEST_CASE("bt2c::StrScanner::_tryScanUnquotedStr() scans an unquoted string")
{
    TestStrScanner ss {"_Hello123 rest"};
    const auto val = ss.tryScanUnquotedStr();

    CHECK(val == "_Hello123");
    CHECK(ss.at() == ss.str().begin() + 9);
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanUnquotedStr() returns an empty view and rewinds when starting with a digit")
{
    TestStrScanner ss {"123abc"};

    CHECK(ss.tryScanUnquotedStr().empty());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanUnquotedStr() returns an empty view when there's nothing left to scan")
{
    TestStrScanner ss {""};

    CHECK(ss.tryScanUnquotedStr().empty());
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanUnquotedStr() considers extra allowed first and other characters")
{
    TestStrScanner ss {"-foo-bar!"};
    const auto val = ss.tryScanUnquotedStr("-", "-");

    CHECK(val == "-foo-bar");
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanUnquotedStr() considers a distinct set of extra first and other characters")
{
    TestStrScanner ss {"$_Ab3-report@2024! rest"};
    const auto val = ss.tryScanUnquotedStr("$@", "-@");

    CHECK(val == "$_Ab3-report@2024");
    CHECK(ss.curChar() == '!');
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanUnquotedStr() doesn't allow an extra `other` character to start the string")
{
    TestStrScanner ss {"-abc"};

    /* `-` is only allowed as an `other` character here, not as a first one */
    CHECK(ss.tryScanUnquotedStr({}, "-").empty());
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::_tryScanUnquotedStr() skips noise (whitespace) before scanning")
{
    TestStrScanner ss {"   foo"};
    const auto val = ss.tryScanUnquotedStr();

    CHECK(val == "foo");
}

TEST_CASE("bt2c::StrScanner::_skipNoise() default implementation does nothing")
{
    DefaultNoiseStrScanner ss {"   abc"};

    ss.skipNoise();
    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("bt2c::StrScanner::_skipWhitespaces() skips whitespace characters, tracking newlines")
{
    TestStrScanner ss {" \t\v\r\n\n abc"};

    ss.skipWhitespaces();
    CHECK(ss.at() == ss.str().end() - 3);
    CHECK(ss.loc().lineNo() == 2);
    CHECK(ss.loc().colNo() == 1);
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanConstReal() scans and decodes a real number matching the regex")
{
    const bt2c::Regex realRegex {"^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?"};
    TestStrScanner ss {"3.1415 rest"};
    const auto val = ss.tryScanConstReal(realRegex);

    REQUIRE(val);
    CHECK(*val == 3.1415);
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanConstReal() scans and decodes a negative integer-like real number")
{
    const bt2c::Regex realRegex {"^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?"};
    TestStrScanner ss {"-42"};
    const auto val = ss.tryScanConstReal(realRegex);

    REQUIRE(val);
    CHECK(*val == -42.0);
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanConstReal() returns `std::nullopt` when the input doesn't match the regex")
{
    const bt2c::Regex realRegex {"^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?"};
    TestStrScanner ss {"nope"};

    CHECK_FALSE(ss.tryScanConstReal(realRegex));
}

TEST_CASE(
    "bt2c::StrScanner::_tryScanConstReal() returns `std::nullopt` when std::strtod() overflows")
{
    const bt2c::Regex realRegex {"^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?"};

    /* Matches the regex, but out of range for `std::strtod()` */
    TestStrScanner ss {"1e400"};

    CHECK_FALSE(ss.tryScanConstReal(realRegex));
}

TEST_CASE("bt2c::StrScanner::_tryScanConstReal() skips noise (whitespace) before scanning")
{
    const bt2c::Regex realRegex {"^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?"};
    TestStrScanner ss {"   2.5"};
    const auto val = ss.tryScanConstReal(realRegex);

    REQUIRE(val);
    CHECK(*val == 2.5);
}

TEST_CASE("`bt2c::StrScannerRejecter` accepts scanned content")
{
    TestStrScanner ss {"abc"};

    {
        bt2c::StrScannerRejecter rejecter {ss};

        ss.incrAt(2);
        rejecter.accept();
    }

    CHECK(ss.at() == ss.str().begin() + 2);
}

TEST_CASE("`bt2c::StrScannerRejecter` rejects scanned content on destruction")
{
    TestStrScanner ss {"abc"};

    {
        bt2c::StrScannerRejecter rejecter {ss};

        ss.incrAt(2);
    }

    CHECK(ss.at() == ss.str().begin());
}

TEST_CASE("`bt2c::StrScannerRejecter` rejects scanned content explicitly")
{
    TestStrScanner ss {"abc"};

    bt2c::StrScannerRejecter rejecter {ss};

    ss.incrAt(2);
    rejecter.reject();
    CHECK(ss.at() == ss.str().begin());
}
