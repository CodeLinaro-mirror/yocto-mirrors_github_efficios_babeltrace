/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstddef>
#include <cstdint>

#include "cpp-common/bt2c/align.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("bt2c::align() with a value which is already aligned")
{
    CHECK(bt2c::align(0U, 4U) == 0U);
    CHECK(bt2c::align(4U, 4U) == 4U);
    CHECK(bt2c::align(1024U, 8U) == 1024U);
}

TEST_CASE("bt2c::align() with a value which isn't aligned")
{
    CHECK(bt2c::align(1U, 4U) == 4U);
    CHECK(bt2c::align(5U, 4U) == 8U);
    CHECK(bt2c::align(7U, 8U) == 8U);
    CHECK(bt2c::align(9U, 8U) == 16U);
}

TEST_CASE("bt2c::align() with an alignment of one")
{
    CHECK(bt2c::align(0U, 1U) == 0U);
    CHECK(bt2c::align(123U, 1U) == 123U);
}

TEST_CASE("bt2c::align() with different value and alignment types")
{
    CHECK(bt2c::align(static_cast<std::size_t>(10), 4) == 12U);
    CHECK(bt2c::align(static_cast<std::uint64_t>(17), static_cast<std::uint8_t>(16)) == 32U);
}
