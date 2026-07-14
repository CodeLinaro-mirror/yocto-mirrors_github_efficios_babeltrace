/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <limits>
#include <string>
#include <string_view>

#include "cpp-common/bt2/error.hpp"
#include "cpp-common/bt2c/parse-json-as-val.hpp"

#include "catch2/catch_test_macros.hpp"

namespace {

const bt2c::Logger theLogger {"test-parse-json", "TEST-PARSE-JSON", bt2c::Logger::Level::None};

bt2c::JsonVal::UP parse(const std::string_view str)
{
    return bt2c::parseJson(str, theLogger);
}

bt2c::JsonVal::UP parse(const std::string_view str, const std::size_t baseOffset)
{
    return bt2c::parseJson(str, baseOffset, theLogger);
}

void requireThrowsWithMsg(const std::string_view json, const std::string& expectedMsg)
{
    REQUIRE_THROWS_AS(parse(json), bt2c::Error);

    const auto error = bt2::takeCurrentThreadError();

    REQUIRE(error);
    CHECK(error[0].message() == expectedMsg);
}

void requireThrowsWithMsgContaining(const std::string_view json, const std::string& expectedSubstr)
{
    REQUIRE_THROWS_AS(parse(json), bt2c::Error);

    const auto error = bt2::takeCurrentThreadError();

    REQUIRE(error);
    CHECK(std::string {error[0].message()}.find(expectedSubstr) != std::string::npos);
}

const std::string expectingValMsg {
    "Expecting a JSON value: `null`, `true`, `false`, a supported number "
    "(for an integer: -9,223,372,036,854,775,808 to 18,446,744,073,709,551,615), "
    "`\"` (a string), `[` (an array), or `{` (an object)."};

/* `expectingValMsg`, prefixed with the text location of offset 0 */
const std::string expectingValAtStartMsg {"[1:1 @ 0 bytes] " + expectingValMsg};

} /* namespace */

TEST_CASE("bt2c::parseJson() parses `null`")
{
    const auto val = parse("null");

    REQUIRE(val);
    CHECK(val->isNull());
    CHECK(val->type() == bt2c::JsonVal::Type::Null);
}

TEST_CASE("bt2c::parseJson() parses `true` and `false`")
{
    {
        const auto val = parse("true");

        REQUIRE(val->isBool());
        CHECK(val->asBool().val());
    }

    {
        const auto val = parse("false");

        REQUIRE(val->isBool());
        CHECK_FALSE(val->asBool().val());
    }
}

TEST_CASE("bt2c::parseJson() parses unsigned integers")
{
    CHECK(parse("0")->asUInt().val() == 0);
    CHECK(parse("42")->asUInt().val() == 42);
    CHECK(parse("1234567890")->asUInt().val() == 1234567890ULL);
    CHECK(parse("18446744073709551615")->asUInt().val() ==
          std::numeric_limits<unsigned long long>::max());
}

TEST_CASE("bt2c::parseJson() parses negative (signed) integers")
{
    CHECK(parse("-1")->asSInt().val() == -1);
    CHECK(parse("-42")->asSInt().val() == -42);
    CHECK(parse("-0")->asSInt().val() == 0);
    CHECK(parse("-9223372036854775808")->asSInt().val() == std::numeric_limits<long long>::min());
}

TEST_CASE("bt2c::parseJson() parses real numbers with a fraction part")
{
    CHECK(parse("3.14")->asReal().val() == 3.14);
    CHECK(parse("0.0")->asReal().val() == 0.0);
    CHECK(parse("-3.14")->asReal().val() == -3.14);
    CHECK(parse("-0.0")->asReal().val() == 0.0);
}

TEST_CASE("bt2c::parseJson() parses real numbers with an exponent part")
{
    CHECK(parse("1e10")->asReal().val() == 1e10);
    CHECK(parse("1E10")->asReal().val() == 1e10);
    CHECK(parse("1e+10")->asReal().val() == 1e10);
    CHECK(parse("1e-10")->asReal().val() == 1e-10);
    CHECK(parse("1.0e+10")->asReal().val() == 1e10);
    CHECK(parse("1.0e-10")->asReal().val() == 1e-10);
}

TEST_CASE("bt2c::parseJson() parses real numbers with both a fraction and an exponent part")
{
    CHECK(parse("2.5e3")->asReal().val() == 2500.0);
    CHECK(parse("-1.5e-2")->asReal().val() == -0.015);
}

TEST_CASE("bt2c::parseJson() rejects malformed or out-of-range numbers")
{
    /* Nothing matches any JSON value grammar rule */
    requireThrowsWithMsg("007.5", expectingValAtStartMsg);
    requireThrowsWithMsg("01.5", expectingValAtStartMsg);

    /* Missing digit after the decimal point */
    requireThrowsWithMsg("1.", expectingValAtStartMsg);

    /* Missing digit in the exponent part */
    requireThrowsWithMsg("1e", expectingValAtStartMsg);

    /* A leading `+` isn't part of the JSON number grammar */
    requireThrowsWithMsg("+1", expectingValAtStartMsg);

    /* Special non-JSON floating point tokens aren't supported */
    requireThrowsWithMsg("NaN", expectingValAtStartMsg);
    requireThrowsWithMsg("Infinity", expectingValAtStartMsg);
    requireThrowsWithMsg("-Infinity", expectingValAtStartMsg);

    /* Unsigned integer overflow (no fraction/exponent part to fall back to) */
    requireThrowsWithMsg("99999999999999999999", expectingValAtStartMsg);

    /* Signed integer underflow (no fraction/exponent part to fall back to) */
    requireThrowsWithMsg("-9223372036854775809", expectingValAtStartMsg);

    /* Real number overflow (exponent too large) */
    requireThrowsWithMsg("1e400", expectingValAtStartMsg);

    /* Real number underflow (exponent too small) */
    requireThrowsWithMsg("1e-400", expectingValAtStartMsg);
}

TEST_CASE("bt2c::parseJson() parses strings without escape sequences")
{
    CHECK(parse(R"("")")->asStr().val() == "");
    CHECK(parse(R"("hello")")->asStr().val() == "hello");
    CHECK(parse(R"("hello, world!")")->asStr().val() == "hello, world!");
}

TEST_CASE("bt2c::parseJson() parses the standard string escape sequences")
{
    CHECK(parse(R"("\"")")->asStr().val() == "\"");
    CHECK(parse(R"("\\")")->asStr().val() == "\\");
    CHECK(parse(R"("\/")")->asStr().val() == "/");
    CHECK(parse(R"("\b")")->asStr().val() == "\b");
    CHECK(parse(R"("\f")")->asStr().val() == "\f");
    CHECK(parse(R"("\n")")->asStr().val() == "\n");
    CHECK(parse(R"("\r")")->asStr().val() == "\r");
    CHECK(parse(R"("\t")")->asStr().val() == "\t");
}

TEST_CASE("bt2c::parseJson() parses `\\u` Unicode escape sequences")
{
    /* One-byte UTF-8 encoding (ASCII, U+0041) */
    CHECK(parse(std::string_view {"\"\\u0041\""})->asStr().val() == "A");

    /* Two-byte UTF-8 encoding (U+00E9) */
    CHECK(parse(std::string_view {"\"\\u00e9\""})->asStr().val() == "\xc3\xa9");

    /* Three-byte UTF-8 encoding (U+4E2D) */
    CHECK(parse(std::string_view {"\"\\u4e2d\""})->asStr().val() == "\xe4\xb8\xad");

    /* Combined with regular (non-escaped) characters */
    CHECK(parse(std::string_view {"\"caf\\u00e9\""})->asStr().val() == "caf\xc3\xa9");

    /* Uppercase hexadecimal digits are also valid */
    CHECK(parse(std::string_view {"\"\\u00E9\""})->asStr().val() == "\xc3\xa9");
}

TEST_CASE("bt2c::parseJson() rejects a raw control character within a string")
{
    requireThrowsWithMsgContaining(std::string {"\"a\tb\""}, "Illegal control character");
}

TEST_CASE("bt2c::parseJson() rejects a truncated `\\u` escape sequence")
{
    requireThrowsWithMsg(R"("\u12")",
                         "[1:2 @ 1 bytes] `\\u` escape sequence needs four hexadecimal digits.");
}

TEST_CASE("bt2c::parseJson() rejects a non-hexadecimal character in a `\\u` escape sequence")
{
    requireThrowsWithMsgContaining(R"("\u12g3")", "unexpected character");
}

TEST_CASE("bt2c::parseJson() rejects an unsupported surrogate codepoint in a `\\u` escape sequence")
{
    /* Both ends of the surrogate range (U+D800 to U+DFFF) are rejected */
    requireThrowsWithMsgContaining(R"("\ud800")", "unsupported surrogate codepoint");
    requireThrowsWithMsgContaining(R"("\ud801")", "unsupported surrogate codepoint");
    requireThrowsWithMsgContaining(R"("\udfff")", "unsupported surrogate codepoint");
}

TEST_CASE("bt2c::parseJson() parses an empty array")
{
    const auto val = parse("[]");

    REQUIRE(val->isArray());
    CHECK(val->asArray().isEmpty());
    CHECK(val->asArray().size() == 0);
}

TEST_CASE("bt2c::parseJson() parses an array of mixed values")
{
    const auto val = parse(R"([1, "two", 3.0, true, null, [4]])");

    REQUIRE(val->isArray());

    auto& arr = val->asArray();

    REQUIRE(arr.size() == 6);
    CHECK(arr[0].asUInt().val() == 1);
    CHECK(arr[1].asStr().val() == "two");
    CHECK(arr[2].asReal().val() == 3.0);
    CHECK(arr[3].asBool().val());
    CHECK(arr[4].isNull());
    REQUIRE(arr[5].isArray());
    CHECK(arr[5].asArray().size() == 1);
    CHECK(arr[5].asArray()[0].asUInt().val() == 4);
}

TEST_CASE("bt2c::parseJson() parses a deeply nested array")
{
    const auto val = parse("[[[[1]]]]");

    const bt2c::JsonVal *cur = val.get();

    for (auto i = 0; i < 3; ++i) {
        REQUIRE(cur->isArray());
        REQUIRE(cur->asArray().size() == 1);
        cur = &cur->asArray()[0];
    }

    REQUIRE(cur->isArray());
    CHECK(cur->asArray()[0].asUInt().val() == 1);
}

TEST_CASE("bt2c::parseJson() parses an empty object")
{
    const auto val = parse("{}");

    REQUIRE(val->isObj());
    CHECK(val->asObj().isEmpty());
    CHECK(val->asObj().size() == 0);
}

TEST_CASE("bt2c::parseJson() parses an object of mixed values, exposing key-based accessors")
{
    const auto val = parse(R"({"i": 42, "s": "hi", "b": true, "n": null, "r": -1})");

    REQUIRE(val->isObj());

    auto& obj = val->asObj();

    CHECK(obj.size() == 5);
    CHECK_FALSE(obj.isEmpty());
    CHECK(obj.hasValue("i"));
    CHECK_FALSE(obj.hasValue("nope"));
    CHECK(obj["i"] != nullptr);
    CHECK(obj["nope"] == nullptr);
    CHECK(obj.rawUIntVal("i") == 42);
    CHECK(obj.rawStrVal("s") == "hi");
    CHECK(obj.rawBoolVal("b"));
    CHECK(obj.val<bt2c::JsonNullVal>("n").isNull());
    CHECK(obj.rawSIntVal("r") == -1);
    CHECK(obj.rawVal("i", 0ULL) == 42);
    CHECK(obj.rawVal("missing", 7ULL) == 7);
    CHECK(std::string {obj.rawVal("s", "def")} == "hi");
    CHECK(std::string {obj.rawVal("missing", "def")} == "def");
}

TEST_CASE("bt2c::parseJson() parses an object with an empty string key")
{
    const auto val = parse(R"({"": 1})");

    REQUIRE(val->isObj());

    auto& obj = val->asObj();

    CHECK(obj.size() == 1);
    CHECK(obj.hasValue(""));
    CHECK(obj.rawUIntVal("") == 1);
}

TEST_CASE("bt2c::parseJson() parses nested objects, allowing the same key at different levels")
{
    const auto val = parse(R"({"a": {"a": 1}})");

    REQUIRE(val->isObj());

    auto& outer = val->asObj();

    REQUIRE(outer.hasValue("a"));
    REQUIRE(outer["a"]->isObj());

    auto& inner = outer["a"]->asObj();

    REQUIRE(inner.hasValue("a"));
    CHECK(inner.rawUIntVal("a") == 1);
}

TEST_CASE("bt2c::parseJson() rejects a duplicate key within the same object")
{
    requireThrowsWithMsgContaining(R"({"a": 1, "a": 2})", "Duplicate JSON object key `a`");
}

TEST_CASE("bt2c::parseJson() rejects a non-string object key")
{
    requireThrowsWithMsg("{1: 2}",
                         "[1:2 @ 1 bytes] Expecting a JSON object key (double-quoted string).");
}

TEST_CASE("bt2c::parseJson() rejects a missing colon between an object key and its value")
{
    requireThrowsWithMsg(R"({"a" 1})", "[1:6 @ 5 bytes] Expecting `:`.");
}

TEST_CASE("bt2c::parseJson() rejects a trailing comma in an array")
{
    requireThrowsWithMsg("[1,]", "[1:4 @ 3 bytes] " + expectingValMsg);
}

TEST_CASE("bt2c::parseJson() rejects a trailing comma in an object")
{
    requireThrowsWithMsg(R"({"a": 1,})",
                         "[1:9 @ 8 bytes] Expecting a JSON object key (double-quoted string).");
}

TEST_CASE("bt2c::parseJson() rejects an unterminated array")
{
    requireThrowsWithMsg("[1,2", "[1:5 @ 4 bytes] Expecting `]`.");
}

TEST_CASE("bt2c::parseJson() rejects an unterminated string")
{
    requireThrowsWithMsg(R"("abc)", expectingValAtStartMsg);
}

TEST_CASE("bt2c::parseJson() rejects an invalid top-level value")
{
    requireThrowsWithMsg("x", expectingValAtStartMsg);
}

TEST_CASE("bt2c::parseJson() rejects extra data after the parsed value")
{
    requireThrowsWithMsg("1 2", "[1:3 @ 2 bytes] Extra data after parsed JSON value.");
}

TEST_CASE("bt2c::parseJson() rejects an empty input")
{
    requireThrowsWithMsg("", expectingValAtStartMsg);
}

TEST_CASE("bt2c::parseJson() skips whitespace around and within a value")
{
    const auto val = parse(" \t\n\r [ 1 , 2 ] \t\n\r ");

    REQUIRE(val->isArray());

    auto& arr = val->asArray();

    REQUIRE(arr.size() == 2);
    CHECK(arr[0].asUInt().val() == 1);
    CHECK(arr[1].asUInt().val() == 2);
}

TEST_CASE("bt2c::parseJson() rejects JSON text with trailing garbage on another line")
{
    requireThrowsWithMsgContaining("{}\n{}", "Extra data after parsed JSON value.");
}

TEST_CASE("bt2c::parseJson() doesn't support C-style comments")
{
    requireThrowsWithMsg("// comment\n1", expectingValAtStartMsg);
}

TEST_CASE("bt2c::parseJson() sets the text location of the root value")
{
    const auto val = parse("  42");
    auto& loc = val->loc();

    CHECK(loc.offset() == 2);
    CHECK(loc.lineNo() == 0);
    CHECK(loc.colNo() == 2);
}

TEST_CASE("bt2c::parseJson() sets text locations correctly across multiple lines")
{
    const auto val = parse("[\n  1,\n  2\n]");

    REQUIRE(val->isArray());

    auto& arr = val->asArray();

    CHECK(val->loc().offset() == 0);
    CHECK(val->loc().lineNo() == 0);
    CHECK(val->loc().colNo() == 0);
    CHECK(arr[0].loc().offset() == 4);
    CHECK(arr[0].loc().lineNo() == 1);
    CHECK(arr[0].loc().colNo() == 2);
    CHECK(arr[1].loc().offset() == 9);
    CHECK(arr[1].loc().lineNo() == 2);
    CHECK(arr[1].loc().colNo() == 2);
}

TEST_CASE("bt2c::parseJson() adds `baseOffset` to the offset of text locations")
{
    const auto val = parse("42", 100);

    CHECK(val->loc().offset() == 100);
    CHECK(val->loc().lineNo() == 0);
    CHECK(val->loc().colNo() == 0);
}

TEST_CASE("bt2c::parseJson() indexes into a parsed array")
{
    const auto val = parse("[10, 20, 30]");

    REQUIRE(val->isArray());

    auto& arr = val->asArray();

    std::size_t i = 0;

    for (auto& elem : arr) {
        CHECK(elem->asUInt().val() == (i + 1) * 10);
        ++i;
    }

    CHECK(i == 3);
}
