/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>
#include <limits>
#include <optional>

#include "cpp-common/bt2c/safe-ops.hpp"

#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

namespace {

template <typename T>
constexpr T minV = std::numeric_limits<T>::min();

template <typename T>
constexpr T maxV = std::numeric_limits<T>::max();

/*
 * 2 to the power of half the bit count of `T`: `halfV<T> - 1` times
 * `halfV<T> + 1` is exactly `maxV<T>`.
 */
template <typename T>
constexpr T halfV = static_cast<T>(T {1} << (std::numeric_limits<T>::digits / 2));

} /* namespace */

/*
 * Unary `+` promotes an `std::uint8_t` operand so that Catch2 prints it
 * as a number instead of as a raw character.
 */

TEMPLATE_TEST_CASE("bt2c::safeMul() with a zero term", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto val = GENERATE(TestType {0}, TestType {1}, maxV<TestType>);

    CAPTURE(+val);

    const auto res1 = bt2c::safeMul<TestType>(0, val);

    REQUIRE(res1);
    CHECK(+*res1 == 0);

    const auto res2 = bt2c::safeMul<TestType>(val, 0);

    REQUIRE(res2);
    CHECK(+*res2 == 0);
}

TEMPLATE_TEST_CASE("bt2c::safeMul() with a product which fits", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b, product] = GENERATE(table<TestType, TestType, TestType>({
        {1, 1, 1},
        {1, maxV<TestType>, maxV<TestType>},
        {2, maxV<TestType> / 2, maxV<TestType> - 1},
        {3, maxV<TestType> / 3, maxV<TestType>},
        {halfV<TestType> - 1, halfV<TestType> + 1, maxV<TestType>},
    }));

    CAPTURE(+a, +b);

    const auto res1 = bt2c::safeMul(a, b);

    REQUIRE(res1);
    CHECK(+*res1 == +product);

    const auto res2 = bt2c::safeMul(b, a);

    REQUIRE(res2);
    CHECK(+*res2 == +product);
}

TEMPLATE_TEST_CASE("bt2c::safeMul() with a product which overflows", "", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {2, maxV<TestType> / 2 + 1},
        {2, maxV<TestType>},
        {3, maxV<TestType> / 3 + 1},
        {halfV<TestType>, halfV<TestType>},
        {maxV<TestType>, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeMul(a, b));
    CHECK_FALSE(bt2c::safeMul(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeAdd() with a sum which fits", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b, sum] = GENERATE(table<TestType, TestType, TestType>({
        {0, 0, 0},
        {0, maxV<TestType>, maxV<TestType>},
        {1, maxV<TestType> - 1, maxV<TestType>},
        {maxV<TestType> / 2, maxV<TestType> / 2 + 1, maxV<TestType>},
    }));

    CAPTURE(+a, +b);

    const auto res1 = bt2c::safeAdd(a, b);

    REQUIRE(res1);
    CHECK(+*res1 == +sum);

    const auto res2 = bt2c::safeAdd(b, a);

    REQUIRE(res2);
    CHECK(+*res2 == +sum);
}

TEMPLATE_TEST_CASE("bt2c::safeAdd() with a sum which overflows", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {1, maxV<TestType>},
        {2, maxV<TestType> - 1},
        {maxV<TestType> / 2 + 1, maxV<TestType> / 2 + 1},
        {maxV<TestType>, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeAdd(a, b));
    CHECK_FALSE(bt2c::safeAdd(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeAdd() with signed terms and a sum which fits", "", std::int8_t,
                   std::int16_t, std::int32_t, std::int64_t)
{
    const auto [a, b, sum] = GENERATE(table<TestType, TestType, TestType>({
        {0, 0, 0},
        {-1, 1, 0},
        {0, maxV<TestType>, maxV<TestType>},
        {0, minV<TestType>, minV<TestType>},
        {-1, maxV<TestType>, maxV<TestType> - 1},
        {1, minV<TestType>, minV<TestType> + 1},
        {minV<TestType>, maxV<TestType>, -1},
        {maxV<TestType> / 2, maxV<TestType> / 2 + 1, maxV<TestType>},
        {minV<TestType> / 2, minV<TestType> / 2, minV<TestType>},
    }));

    CAPTURE(+a, +b);

    const auto res1 = bt2c::safeAdd(a, b);

    REQUIRE(res1);
    CHECK(+*res1 == +sum);

    const auto res2 = bt2c::safeAdd(b, a);

    REQUIRE(res2);
    CHECK(+*res2 == +sum);
}

TEMPLATE_TEST_CASE("bt2c::safeAdd() with signed terms and a sum which overflows", "", std::int8_t,
                   std::int16_t, std::int32_t, std::int64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {1, maxV<TestType>},
        {maxV<TestType> / 2 + 1, maxV<TestType> / 2 + 1},
        {maxV<TestType>, maxV<TestType>},
        {-1, minV<TestType>},
        {minV<TestType> / 2 - 1, minV<TestType> / 2},
        {minV<TestType>, minV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeAdd(a, b));
    CHECK_FALSE(bt2c::safeAdd(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeSub() with a difference which fits", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b, diff] = GENERATE(table<TestType, TestType, TestType>({
        {0, 0, 0},
        {1, 0, 1},
        {1, 1, 0},
        {maxV<TestType>, 0, maxV<TestType>},
        {maxV<TestType>, 1, maxV<TestType> - 1},
        {maxV<TestType>, maxV<TestType>, 0},
    }));

    CAPTURE(+a, +b);

    const auto res = bt2c::safeSub(a, b);

    REQUIRE(res);
    CHECK(+*res == +diff);
}

TEMPLATE_TEST_CASE("bt2c::safeSub() with a difference which overflows", "", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {0, 1},
        {1, 2},
        {0, maxV<TestType>},
        {maxV<TestType> - 1, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeSub(a, b));
}

TEMPLATE_TEST_CASE("bt2c::safeSub() with signed terms and a difference which fits", "", std::int8_t,
                   std::int16_t, std::int32_t, std::int64_t)
{
    const auto [a, b, diff] = GENERATE(table<TestType, TestType, TestType>({
        {0, 0, 0},
        {0, maxV<TestType>, -maxV<TestType>},
        {-1, maxV<TestType>, minV<TestType>},
        {-1, minV<TestType>, maxV<TestType>},
        {maxV<TestType>, 0, maxV<TestType>},
        {maxV<TestType> - 1, -1, maxV<TestType>},
        {maxV<TestType>, maxV<TestType>, 0},
        {minV<TestType>, 0, minV<TestType>},
        {minV<TestType>, -1, minV<TestType> + 1},
        {minV<TestType>, minV<TestType>, 0},
    }));

    CAPTURE(+a, +b);

    const auto res = bt2c::safeSub(a, b);

    REQUIRE(res);
    CHECK(+*res == +diff);
}

TEMPLATE_TEST_CASE("bt2c::safeSub() with signed terms and a difference which overflows", "",
                   std::int8_t, std::int16_t, std::int32_t, std::int64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {0, minV<TestType>},
        {maxV<TestType>, -1},
        {maxV<TestType>, minV<TestType>},
        {minV<TestType>, 1},
        {-2, maxV<TestType>},
        {minV<TestType>, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeSub(a, b));
}

TEST_CASE("bt2c::safeMul(), bt2c::safeAdd(), and bt2c::safeSub() with all 8-bit terms")
{
    constexpr unsigned int max = maxV<std::uint8_t>;
    unsigned int mismatchCount = 0;

    /*
     * Returns whether or not `res` matches `expected`, the result of
     * the same operation on `unsigned int`, which can't overflow.
     */
    const auto matches = [](const std::optional<std::uint8_t>& res, const unsigned int expected) {
        if (expected > max) {
            return !res;
        }

        return res && *res == expected;
    };

    for (unsigned int a = 0; a <= max; ++a) {
        for (unsigned int b = 0; b <= max; ++b) {
            const auto a8 = static_cast<std::uint8_t>(a);
            const auto b8 = static_cast<std::uint8_t>(b);

            if (!matches(bt2c::safeMul(a8, b8), a * b)) {
                UNSCOPED_INFO("bt2c::safeMul(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }

            if (!matches(bt2c::safeAdd(a8, b8), a + b)) {
                UNSCOPED_INFO("bt2c::safeAdd(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }

            /* `a - b` wraps when `b > a`, making it greater than `max` */
            if (!matches(bt2c::safeSub(a8, b8), a - b)) {
                UNSCOPED_INFO("bt2c::safeSub(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }
        }
    }

    CHECK(mismatchCount == 0);
}

TEST_CASE("bt2c::safeAdd() and bt2c::safeSub() with all signed 8-bit terms")
{
    constexpr int min = minV<std::int8_t>;
    constexpr int max = maxV<std::int8_t>;
    unsigned int mismatchCount = 0;

    /*
     * Returns whether or not `res` matches `expected`, the result of
     * the same operation on `int`, which can't overflow.
     */
    const auto matches = [](const std::optional<std::int8_t>& res, const int expected) {
        if (expected < min || expected > max) {
            return !res;
        }

        return res && *res == expected;
    };

    for (int a = min; a <= max; ++a) {
        for (int b = min; b <= max; ++b) {
            const auto a8 = static_cast<std::int8_t>(a);
            const auto b8 = static_cast<std::int8_t>(b);

            if (!matches(bt2c::safeAdd(a8, b8), a + b)) {
                UNSCOPED_INFO("bt2c::safeAdd(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }

            if (!matches(bt2c::safeSub(a8, b8), a - b)) {
                UNSCOPED_INFO("bt2c::safeSub(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }
        }
    }

    CHECK(mismatchCount == 0);
}

TEST_CASE("bt2c::safeMul(), bt2c::safeAdd(), and bt2c::safeSub() are usable at compile time")
{
    STATIC_CHECK(*bt2c::safeMul<std::uint8_t>(15, 17) == 255);
    STATIC_CHECK(!bt2c::safeMul<std::uint8_t>(16, 16));
    STATIC_CHECK(*bt2c::safeAdd<std::uint8_t>(128, 127) == 255);
    STATIC_CHECK(!bt2c::safeAdd<std::uint8_t>(128, 128));
    STATIC_CHECK(*bt2c::safeSub<std::uint8_t>(255, 255) == 0);
    STATIC_CHECK(!bt2c::safeSub<std::uint8_t>(0, 1));
    STATIC_CHECK(*bt2c::safeAdd<std::int8_t>(-128, 127) == -1);
    STATIC_CHECK(!bt2c::safeAdd<std::int8_t>(-128, -1));
    STATIC_CHECK(*bt2c::safeSub<std::int8_t>(-1, -128) == 127);
    STATIC_CHECK(!bt2c::safeSub<std::int8_t>(0, -128));
}

TEMPLATE_TEST_CASE(
    "bt2c::safeMulDbg(), bt2c::safeAddDbg(), and bt2c::safeSubDbg() return the result", "",
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t)
{
    constexpr auto max = maxV<TestType>;
    constexpr auto half = halfV<TestType>;

    CHECK(+bt2c::safeMulDbg<TestType>(0, max) == 0);
    CHECK(+bt2c::safeMulDbg<TestType>(1, max) == +max);
    CHECK(+bt2c::safeMulDbg<TestType>(3, max / 3) == +max);
    CHECK(+bt2c::safeMulDbg<TestType>(half - 1, half + 1) == +max);
    CHECK(+bt2c::safeAddDbg<TestType>(0, max) == +max);
    CHECK(+bt2c::safeAddDbg<TestType>(max / 2, max / 2 + 1) == +max);
    CHECK(+bt2c::safeSubDbg<TestType>(max, 0) == +max);
    CHECK(+bt2c::safeSubDbg<TestType>(max, 1) == max - 1);
    CHECK(+bt2c::safeSubDbg<TestType>(max, max) == 0);
}

TEMPLATE_TEST_CASE("bt2c::safeAddDbg() and bt2c::safeSubDbg() return the result with signed terms",
                   "", std::int8_t, std::int16_t, std::int32_t, std::int64_t)
{
    constexpr auto min = minV<TestType>;
    constexpr auto max = maxV<TestType>;

    CHECK(+bt2c::safeAddDbg<TestType>(min, max) == -1);
    CHECK(+bt2c::safeAddDbg<TestType>(-1, min + 1) == +min);
    CHECK(+bt2c::safeSubDbg<TestType>(-1, min) == +max);
    CHECK(+bt2c::safeSubDbg<TestType>(min + 1, 1) == +min);
}
