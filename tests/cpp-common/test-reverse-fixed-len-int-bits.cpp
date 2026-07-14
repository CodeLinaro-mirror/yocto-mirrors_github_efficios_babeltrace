/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>

#include "cpp-common/bt2c/data-len.hpp"
#include "cpp-common/bt2c/reverse-fixed-len-int-bits.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("bt2c::reverseFixedLenIntBits() with an unsigned value")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(0b111011010),
                                       bt2c::DataLen::fromBits(9)) == 0b010110111ULL);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() with a signed value sign-extends the result")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::int64_t>(0b01011),
                                       bt2c::DataLen::fromBits(5)) == -6);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() with a single bit doesn't change the value")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(1), bt2c::DataLen::fromBits(1)) ==
          1ULL);
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(0), bt2c::DataLen::fromBits(1)) ==
          0ULL);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() moves the least significant bit to the most significant "
          "position")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(0b1),
                                       bt2c::DataLen::fromBits(8)) == 0b10000000ULL);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() is its own inverse over the same length")
{
    const auto val = static_cast<std::uint64_t>(0b111011010);
    const auto len = bt2c::DataLen::fromBits(9);

    CHECK(bt2c::reverseFixedLenIntBits(bt2c::reverseFixedLenIntBits(val, len), len) == val);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() with a full 64-bit length reverses all bits")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(1),
                                       bt2c::DataLen::fromBits(64)) == 0x8000000000000000ULL);
    CHECK(bt2c::reverseFixedLenIntBits(0xffffffffffffffffULL, bt2c::DataLen::fromBits(64)) ==
          0xffffffffffffffffULL);
}

TEST_CASE("bt2c::reverseFixedLenIntBits() with an all-zero value returns zero")
{
    CHECK(bt2c::reverseFixedLenIntBits(static_cast<std::uint64_t>(0),
                                       bt2c::DataLen::fromBits(32)) == 0ULL);
}
