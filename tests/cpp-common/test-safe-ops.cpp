/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>
#include <limits>

#include "cpp-common/bt2c/safe-ops.hpp"

#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

namespace {

template <typename T>
constexpr T maxV = std::numeric_limits<T>::max();

/*
 * 2 to the power of half the bit count of `T`: `halfV<T> - 1` times
 * `halfV<T> + 1` is exactly `maxV<T>`.
 */
template <typename T>
constexpr T halfV = static_cast<T>(T {1} << (std::numeric_limits<T>::digits / 2));

} /* namespace */

TEMPLATE_TEST_CASE("bt2c::safeToMul() with a zero term", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto val = GENERATE(TestType {0}, TestType {1}, maxV<TestType>);

    CAPTURE(+val);
    CHECK(bt2c::safeToMul<TestType>(0, val));
    CHECK(bt2c::safeToMul<TestType>(val, 0));
}

TEMPLATE_TEST_CASE("bt2c::safeToMul() with a product which fits", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {1, 1},
        {1, maxV<TestType>},
        {2, maxV<TestType> / 2},
        {3, maxV<TestType> / 3},
        {halfV<TestType> - 1, halfV<TestType> + 1},
    }));

    CAPTURE(+a, +b);
    CHECK(bt2c::safeToMul(a, b));
    CHECK(bt2c::safeToMul(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeToMul() with a product which overflows", "", std::uint8_t,
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
    CHECK_FALSE(bt2c::safeToMul(a, b));
    CHECK_FALSE(bt2c::safeToMul(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeToAdd() with a sum which fits", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {0, 0},
        {0, maxV<TestType>},
        {1, maxV<TestType> - 1},
        {maxV<TestType> / 2, maxV<TestType> / 2 + 1},
    }));

    CAPTURE(+a, +b);
    CHECK(bt2c::safeToAdd(a, b));
    CHECK(bt2c::safeToAdd(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeToAdd() with a sum which overflows", "", std::uint8_t, std::uint16_t,
                   std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {1, maxV<TestType>},
        {2, maxV<TestType> - 1},
        {maxV<TestType> / 2 + 1, maxV<TestType> / 2 + 1},
        {maxV<TestType>, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeToAdd(a, b));
    CHECK_FALSE(bt2c::safeToAdd(b, a));
}

TEMPLATE_TEST_CASE("bt2c::safeToSub() with a difference which fits", "", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {0, 0},
        {1, 0},
        {1, 1},
        {maxV<TestType>, 0},
        {maxV<TestType>, 1},
        {maxV<TestType>, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK(bt2c::safeToSub(a, b));
}

TEMPLATE_TEST_CASE("bt2c::safeToSub() with a difference which overflows", "", std::uint8_t,
                   std::uint16_t, std::uint32_t, std::uint64_t)
{
    const auto [a, b] = GENERATE(table<TestType, TestType>({
        {0, 1},
        {1, 2},
        {0, maxV<TestType>},
        {maxV<TestType> - 1, maxV<TestType>},
    }));

    CAPTURE(+a, +b);
    CHECK_FALSE(bt2c::safeToSub(a, b));
}

TEST_CASE("bt2c::safeToMul(), bt2c::safeToAdd(), and bt2c::safeToSub() with all 8-bit terms")
{
    constexpr unsigned int max = maxV<std::uint8_t>;
    unsigned int mismatchCount = 0;

    /* Compare to the operations on `unsigned int`, which can't overflow */
    for (unsigned int a = 0; a <= max; ++a) {
        for (unsigned int b = 0; b <= max; ++b) {
            const auto a8 = static_cast<std::uint8_t>(a);
            const auto b8 = static_cast<std::uint8_t>(b);

            if (bt2c::safeToMul(a8, b8) != (a * b <= max)) {
                UNSCOPED_INFO("bt2c::safeToMul(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }

            if (bt2c::safeToAdd(a8, b8) != (a + b <= max)) {
                UNSCOPED_INFO("bt2c::safeToAdd(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }

            if (bt2c::safeToSub(a8, b8) != (a >= b)) {
                UNSCOPED_INFO("bt2c::safeToSub(" << a << ", " << b << ") is wrong");
                ++mismatchCount;
            }
        }
    }

    CHECK(mismatchCount == 0);
}

TEMPLATE_TEST_CASE("bt2c::safeMul(), bt2c::safeAdd(), and bt2c::safeSub() return the result", "",
                   std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t)
{
    constexpr auto max = maxV<TestType>;
    constexpr auto half = halfV<TestType>;

    /*
     * Unary `+` promotes an `std::uint8_t` operand so that Catch2
     * prints it as a number instead of as a raw character.
     */
    CHECK(+bt2c::safeMul<TestType>(0, max) == 0);
    CHECK(+bt2c::safeMul<TestType>(1, max) == +max);
    CHECK(+bt2c::safeMul<TestType>(3, max / 3) == +max);
    CHECK(+bt2c::safeMul<TestType>(half - 1, half + 1) == +max);
    CHECK(+bt2c::safeAdd<TestType>(0, max) == +max);
    CHECK(+bt2c::safeAdd<TestType>(max / 2, max / 2 + 1) == +max);
    CHECK(+bt2c::safeSub<TestType>(max, 0) == +max);
    CHECK(+bt2c::safeSub<TestType>(max, 1) == max - 1);
    CHECK(+bt2c::safeSub<TestType>(max, max) == 0);
}
