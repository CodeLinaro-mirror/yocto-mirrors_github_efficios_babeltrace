/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>
#include <limits>

#include "common/safe.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

namespace {

constexpr auto int64Min = std::numeric_limits<std::int64_t>::min();
constexpr auto int64Max = std::numeric_limits<std::int64_t>::max();
constexpr auto uInt64Max = std::numeric_limits<std::uint64_t>::max();

/* Largest positive integer of which the square is at most `int64Max` */
constexpr std::int64_t int64MaxSqrt = 3'037'000'499;

} /* namespace */

TEST_CASE("bt_safe_to_mul_int64() with a zero term")
{
    const auto val =
        GENERATE(std::int64_t {0}, std::int64_t {1}, std::int64_t {-1}, int64Min, int64Max);

    CAPTURE(val);
    CHECK(bt_safe_to_mul_int64(0, val));
    CHECK(bt_safe_to_mul_int64(val, 0));
}

TEST_CASE("bt_safe_to_mul_int64() with a product which fits")
{
    const auto [a, b] = GENERATE(table<std::int64_t, std::int64_t>({
        /* Positive product */
        {1, 1},
        {1, int64Max},
        {2, int64Max / 2},
        {int64MaxSqrt, int64MaxSqrt},

        /* Negative product */
        {1, -1},
        {1, int64Min},
        {-1, int64Max},
        {2, int64Min / 2},
        {-2, int64Max / 2 + 1},
        {int64MaxSqrt, -int64MaxSqrt},

        /* Positive product of two negative terms */
        {-1, -1},
        {-1, int64Min + 1},
        {-2, int64Min / 2 + 1},
        {-int64MaxSqrt, -int64MaxSqrt},
    }));

    CAPTURE(a, b);
    CHECK(bt_safe_to_mul_int64(a, b));
    CHECK(bt_safe_to_mul_int64(b, a));
}

TEST_CASE("bt_safe_to_mul_int64() with a product which overflows")
{
    const auto [a, b] = GENERATE(table<std::int64_t, std::int64_t>({
        /* Positive product */
        {2, int64Max / 2 + 1},
        {2, int64Max},
        {int64Max, int64Max},
        {int64MaxSqrt + 1, int64MaxSqrt + 1},

        /* Negative product */
        {2, int64Min / 2 - 1},
        {2, int64Min},
        {-2, int64Max / 2 + 2},
        {-2, int64Max},
        {int64Max, int64Min},
        {int64MaxSqrt + 2, -int64MaxSqrt - 2},

        /* Positive product of two negative terms */
        {-1, int64Min},
        {-2, int64Min / 2},
        {int64Min, int64Min},
        {-int64MaxSqrt - 1, -int64MaxSqrt - 1},
    }));

    CAPTURE(a, b);
    CHECK_FALSE(bt_safe_to_mul_int64(a, b));
    CHECK_FALSE(bt_safe_to_mul_int64(b, a));
}

TEST_CASE("bt_safe_to_mul_uint64() with a zero term")
{
    const auto val = GENERATE(std::uint64_t {0}, std::uint64_t {1}, uInt64Max);

    CAPTURE(val);
    CHECK(bt_safe_to_mul_uint64(0, val));
    CHECK(bt_safe_to_mul_uint64(val, 0));
}

TEST_CASE("bt_safe_to_mul_uint64() with a product which fits")
{
    const auto [a, b] = GENERATE(table<std::uint64_t, std::uint64_t>({
        {1, 1},
        {1, uInt64Max},
        {2, uInt64Max / 2},
        {3, uInt64Max / 3},
        {0xffffffff, 0x100000001},
    }));

    CAPTURE(a, b);
    CHECK(bt_safe_to_mul_uint64(a, b));
    CHECK(bt_safe_to_mul_uint64(b, a));
}

TEST_CASE("bt_safe_to_mul_uint64() with a product which overflows")
{
    const auto [a, b] = GENERATE(table<std::uint64_t, std::uint64_t>({
        {2, uInt64Max / 2 + 1},
        {2, uInt64Max},
        {3, uInt64Max / 3 + 1},
        {0x100000000, 0x100000000},
        {uInt64Max, uInt64Max},
    }));

    CAPTURE(a, b);
    CHECK_FALSE(bt_safe_to_mul_uint64(a, b));
    CHECK_FALSE(bt_safe_to_mul_uint64(b, a));
}

TEST_CASE("bt_safe_to_add_int64() with a sum which fits")
{
    const auto [a, b] = GENERATE(table<std::int64_t, std::int64_t>({
        {0, 0},
        {0, int64Max},
        {0, int64Min},
        {0, -1},
        {1, int64Max - 1},
        {-1, -1},
        {-1, int64Min + 1},
        {-1, int64Max},
        {1, int64Min},
        {int64Min, int64Max},
        {int64Max / 2, int64Max / 2 + 1},
        {int64Min / 2, int64Min / 2},
    }));

    CAPTURE(a, b);
    CHECK(bt_safe_to_add_int64(a, b));
    CHECK(bt_safe_to_add_int64(b, a));
}

TEST_CASE("bt_safe_to_add_int64() with a sum which overflows")
{
    const auto [a, b] = GENERATE(table<std::int64_t, std::int64_t>({
        {1, int64Max},
        {2, int64Max - 1},
        {int64Max, int64Max},
        {int64Max / 2 + 1, int64Max / 2 + 1},
        {-1, int64Min},
        {-2, int64Min + 1},
        {int64Min, int64Min},
        {int64Min / 2, int64Min / 2 - 1},
    }));

    CAPTURE(a, b);
    CHECK_FALSE(bt_safe_to_add_int64(a, b));
    CHECK_FALSE(bt_safe_to_add_int64(b, a));
}

TEST_CASE("bt_safe_to_add_uint64() with a sum which fits")
{
    const auto [a, b] = GENERATE(table<std::uint64_t, std::uint64_t>({
        {0, 0},
        {0, uInt64Max},
        {1, uInt64Max - 1},
        {uInt64Max / 2, uInt64Max / 2 + 1},
    }));

    CAPTURE(a, b);
    CHECK(bt_safe_to_add_uint64(a, b));
    CHECK(bt_safe_to_add_uint64(b, a));
}

TEST_CASE("bt_safe_to_add_uint64() with a sum which overflows")
{
    const auto [a, b] = GENERATE(table<std::uint64_t, std::uint64_t>({
        {1, uInt64Max},
        {2, uInt64Max - 1},
        {uInt64Max / 2 + 1, uInt64Max / 2 + 1},
        {uInt64Max, uInt64Max},
    }));

    CAPTURE(a, b);
    CHECK_FALSE(bt_safe_to_add_uint64(a, b));
    CHECK_FALSE(bt_safe_to_add_uint64(b, a));
}
