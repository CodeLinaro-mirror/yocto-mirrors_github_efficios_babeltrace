/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016-2019 Philippe Proulx <pproulx@efficios.com>
 */

#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <fmt/format.h>

#include <babeltrace2/babeltrace.h>

#include "common/assert.h"
#include "cpp-common/bt2/value.hpp"
#include "cpp-common/bt2c/exc.hpp"
#include "cpp-common/bt2c/logging.hpp"
#include "cpp-common/bt2c/regex.hpp"
#include "cpp-common/bt2c/str-scanner.hpp"

#include "param-parse.h"

namespace {

/*
 * String scanner specialized for the `--params` argument grammar.
 */
class ParamsStrScanner final : public bt2c::StrScanner
{
public:
    explicit ParamsStrScanner(const std::string_view str, const bt2c::Logger& logger)
        : bt2c::StrScanner {str, logger}
    {
    }

    /*
     * Tries to scan a map key or unquoted string
     * value (`[a-zA-Z_][a-zA-Z0-9_.:-]*`).
     */
    std::string_view tryScanIdent() noexcept
    {
        return this->_tryScanUnquotedStr({}, ".:-");
    }

    /*
     * Tries to scan a double-quoted literal string, supporting the
     * `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, and `\v` single-character
     * escape sequences, as well as octal (`\0` to `\7`) ones.
     */
    std::optional<std::string_view> tryScanLitStr()
    {
        return this->_tryScanLitStr("abfnrtv0");
    }

    /*
     * Tries to scan and decode a constant real number string, C style,
     * allowing a missing integer or fractional part (unlike JSON).
     */
    std::optional<double> tryScanConstReal() noexcept
    {
        return this->_tryScanConstReal(_realRegex);
    }

private:
    /*
     * Only whitespaces are noise between constructs of this grammar.
     */
    void _skipNoise() noexcept override
    {
        this->_skipWhitespaces();
    }

    /* clang-format off */

    /* Constant real number string regex (C style, missing parts allowed) */
    static inline const bt2c::Regex _realRegex {
        "^"                             /* Start of target */
        "-?"                            /* Optional negation */
        "(?:"
            "(?:\\d+\\.\\d*|\\.\\d+)"   /* Integer and/or fraction part, with a `.` */
            "(?:[eE][+-]?\\d+)?"        /* Optional exponent part */
            "|"
            "\\d+[eE][+-]?\\d+"         /* Integer part and mandatory exponent part, no `.` */
        ")"
    };

    /* clang-format on */
};

/*
 * A recursive-descent parser of the `--params` argument syntax.
 *
 * Throws `bt2c::Error` with a human-readable message when a scanned
 * integer doesn't fit a 64-bit signed integer where required.
 *
 * Throws `bt2c::MemoryError` on memory error.
 */
class ParamsParser final
{
public:
    explicit ParamsParser(const std::string_view arg)
        : _mArg {arg},
          _mLogger {"PARAM-PARSER", "PARAM-PARSER", bt2c::Logger::Level::Warning},
          _mSs {arg, _mLogger}
    {
    }

    /*
     * Parses the whole wrapped argument as a map of entries, expecting
     * at least one `KEY=VAL` entry, more entries separated with `,`,
     * and nothing else (no trailing comma).
     */
    bt2::MapValue::Shared parse()
    {
        auto paramsVal = bt2::MapValue::create();

        this->_parseMapEntry(*paramsVal);

        while (_mSs.tryScanToken(",")) {
            this->_parseMapEntry(*paramsVal);
        }

        _mSs.skipNoise();

        if (!_mSs.isDone()) {
            this->_errorExpecting("`,`");
        }

        return paramsVal;
    }

private:
    /*
     * A scanned, not-yet-negated constant number: either an unsigned
     * integer magnitude or a real number.
     */
    using _Number = std::variant<std::uint64_t, double>;

    /*
     * Parses a single `KEY=VAL` map entry, inserting it into `mapVal`.
     */
    void _parseMapEntry(const bt2::MapValue mapVal)
    {
        const auto key = this->_expectMapKey();

        if (!_mSs.tryScanToken("=")) {
            this->_errorExpecting("`=`");
        }

        mapVal.insert(key, *this->_expectValue());
    }

    std::string _expectMapKey()
    {
        const auto key = _mSs.tryScanIdent();

        if (key.empty()) {
            this->_errorExpecting("an unquoted map key");
        }

        return std::string {key};
    }

    /*
     * Expects, parses, and returns a single value: a negative number,
     * an explicitly unsigned integer, an array, a map, a literal
     * string, a positive number, or an unquoted string value.
     */
    bt2::Value::Shared _expectValue()
    {
        _mSs.skipNoise();

        if (_mSs.isDone()) {
            this->_errorExpecting("a value");
        }

        const auto c = _mSs.curChar();

        if (_mSs.tryScanToken("-")) {
            return this->_expectNegNumber();
        }

        if (_mSs.tryScanToken("+")) {
            return this->_expectUInt();
        }

        if (c == '[') {
            return this->_expectArray();
        }

        if (c == '{') {
            return this->_expectMap();
        }

        if (c == '"') {
            return this->_expectLitStr();
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            if (const auto num = this->_tryScanNumber()) {
                return this->_valueFromNumber(*num, false);
            }

            this->_errorExpecting("a value");
        }

        return this->_expectUnquotedStrValue();
    }

    /*
     * Expects and parses an array value, of which the opening `[` is
     * the current character.
     *
     * Supports an optional trailing comma.
     */
    bt2::Value::Shared _expectArray()
    {
        _mSs.tryScanToken("[");

        auto arrayVal = bt2::ArrayValue::create();

        if (_mSs.tryScanToken("]")) {
            /* Empty */
            return arrayVal;
        }

        while (true) {
            arrayVal->append(*this->_expectValue());

            if (_mSs.tryScanToken(",")) {
                if (_mSs.tryScanToken("]")) {
                    break;
                }

                continue;
            }

            if (!_mSs.tryScanToken("]")) {
                this->_errorExpecting("`,` or `]`");
            }

            break;
        }

        return arrayVal;
    }

    /*
     * Expects and parses a map value, of which the opening `{` is the
     * current character.
     *
     * Supports an optional trailing comma.
     */
    bt2::Value::Shared _expectMap()
    {
        _mSs.tryScanToken("{");

        auto mapVal = bt2::MapValue::create();

        if (_mSs.tryScanToken("}")) {
            return mapVal;
        }

        while (true) {
            this->_parseMapEntry(*mapVal);

            if (_mSs.tryScanToken(",")) {
                if (_mSs.tryScanToken("}")) {
                    break;
                }

                continue;
            }

            if (!_mSs.tryScanToken("}")) {
                this->_errorExpecting("`,` or `}`");
            }

            break;
        }

        return mapVal;
    }

    bt2::Value::Shared _expectLitStr()
    {
        std::optional<std::string_view> str;

        try {
            str = _mSs.tryScanLitStr();
        } catch (const std::exception&) {
            this->_errorExpecting("a valid double-quoted string");
        }

        if (!str) {
            this->_errorExpecting("a value");
        }

        return bt2::StringValue::create(std::string {*str});
    }

    /*
     * Expects and parses an unquoted string value: `null`-like, a
     * `true`/`false`-like boolean, or, as a fallback, a bare string.
     */
    bt2::Value::Shared _expectUnquotedStrValue()
    {
        const auto str = _mSs.tryScanIdent();

        if (str.empty()) {
            this->_errorExpecting("a value");
        } else if (str == "null" || str == "NULL" || str == "nul") {
            return bt2::NullValue {}.shared();
        } else if (str == "true" || str == "TRUE" || str == "yes" || str == "YES") {
            return bt2::BoolValue::create(true);
        } else if (str == "false" || str == "FALSE" || str == "no" || str == "NO") {
            return bt2::BoolValue::create(false);
        } else {
            return bt2::StringValue::create(std::string {str});
        }
    }

    /* Expects a number and returns its negation. */
    bt2::Value::Shared _expectNegNumber()
    {
        if (const auto num = this->_tryScanNumber()) {
            return this->_valueFromNumber(*num, true);
        }

        this->_errorExpecting("a value");
    }

    /*
     * Expects an explicitly unsigned integer (_no_ real number).
     */
    bt2::Value::Shared _expectUInt()
    {
        const auto num = this->_tryScanNumber();

        if (!num || std::holds_alternative<double>(*num)) {
            this->_errorExpecting("an integer value");
        }

        return bt2::UnsignedIntegerValue::create(std::get<std::uint64_t>(*num));
    }

    /*
     * Returns the value corresponding to the scanned number `num`,
     * negated if `negate` is true.
     *
     * Throws if `num` is a non-real magnitude which doesn't fit a
     * 64-bit signed integer once possibly negated.
     */
    bt2::Value::Shared _valueFromNumber(const _Number& num, const bool negate)
    {
        if (const auto fVal = std::get_if<double>(&num)) {
            return bt2::RealValue::create(negate ? -*fVal : *fVal);
        }

        const auto uVal = std::get<std::uint64_t>(num);
        static constexpr auto int64Max =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());

        if (negate) {
            if (uVal > int64Max + 1) {
                this->_error(fmt::format(
                    "Integer value -{} is outside the range of a 64-bit signed integer", uVal));
            }

            return bt2::SignedIntegerValue::create(-static_cast<std::int64_t>(uVal));
        }

        if (uVal > int64Max) {
            this->_error(fmt::format(
                "Integer value {} is outside the range of a 64-bit signed integer", uVal));
        }

        return bt2::SignedIntegerValue::create(static_cast<std::int64_t>(uVal));
    }

    /*
     * Tries to scan a constant number magnitude (never negated; a
     * leading `-` is handled by the caller): a floating point number
     * (with an optional missing integer or fractional part, unlike
     * JSON), or, as a fallback, an unsigned integer (`0b`/`0B` binary,
     * `0x`/`0X` hexadecimal, leading-zero octal, or decimal).
     */
    std::optional<_Number> _tryScanNumber() noexcept
    {
        if (const auto val = _mSs.tryScanConstReal()) {
            return _Number {*val};
        }

        if (const auto val = _mSs.tryScanConstUInt<true>()) {
            return _Number {static_cast<std::uint64_t>(*val)};
        }

        return std::nullopt;
    }

    [[noreturn]] void _errorExpecting(const std::string_view what) const
    {
        this->_error(fmt::format("Expecting {}", what));
    }

    /*
     * Throws `bt2c::Error` with the message `what`, followed, for a
     * single-line non-empty argument, with the argument itself and a
     * `^` marker below the current scanning position.
     */
    [[noreturn]] void _error(const std::string& what) const
    {
        auto msg = fmt::format("{}:\n", what);

        if (!_mArg.empty() && _mArg.find('\n') == std::string_view::npos) {
            msg += fmt::format("\n    {}\n    ", _mArg);
            msg.append(static_cast<std::string::size_type>(_mSs.loc().offset()), ' ');
            msg += '^';
        }

        throw bt2c::Error {std::move(msg)};
    }

private:
    /* Complete string to parse, for error messages */
    std::string_view _mArg;

    /* Logging configuration, needed to build `_mSs` */
    bt2c::Logger _mLogger;

    /* Lexical scanner */
    ParamsStrScanner _mSs;
};

} /* namespace */

bt_value *bt_param_parse(const char * const arg, GString * const error)
{
    BT_ASSERT(error);
    g_string_assign(error, "");

    try {
        ParamsParser parser {arg};

        return parser.parse().release().libObjPtr();
    } catch (const bt2c::MemoryError&) {
        g_string_append(error, "Out of memory\n");
    } catch (const std::exception& exc) {
        g_string_append(error, exc.what());
    }

    /*
     * An exception thrown by `bt2c::StrScanner` may have appended a
     * cause to the error of the current thread: clear it so that this
     * function remains free of side effects on that error, as
     * documented by its `error` parameter.
     */
    bt_current_thread_clear_error();
    return nullptr;
}
