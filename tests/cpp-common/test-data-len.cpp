/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include "cpp-common/bt2c/data-len.hpp"

#include "catch2/catch_test_macros.hpp"

using namespace bt2c::literals::datalen;

TEST_CASE("bt2c::DataLen::fromBits() sets the expected number of bits")
{
    CHECK(bt2c::DataLen::fromBits(10).bits() == 10);
}

TEST_CASE("bt2c::DataLen::fromBytes() sets the expected number of bits")
{
    CHECK(bt2c::DataLen::fromBytes(4).bits() == 32);
}

TEST_CASE("bt2c::DataLen::bytes() floors the number of bytes")
{
    CHECK(bt2c::DataLen::fromBits(10).bytes() == 1);
    CHECK(bt2c::DataLen::fromBits(16).bytes() == 2);
    CHECK(bt2c::DataLen::fromBits(23).bytes() == 2);
}

TEST_CASE("bt2c::DataLen::hasExtraBits() with extra bits")
{
    CHECK(bt2c::DataLen::fromBits(10).hasExtraBits());
}

TEST_CASE("bt2c::DataLen::hasExtraBits() without extra bits")
{
    CHECK_FALSE(bt2c::DataLen::fromBits(16).hasExtraBits());
    CHECK_FALSE(bt2c::DataLen::fromBits(0).hasExtraBits());
}

TEST_CASE("bt2c::DataLen::extraBitCount() returns the expected value")
{
    CHECK(bt2c::DataLen::fromBits(10).extraBitCount() == 2);
    CHECK(bt2c::DataLen::fromBits(16).extraBitCount() == 0);
    CHECK(bt2c::DataLen::fromBits(23).extraBitCount() == 7);
}

TEST_CASE("bt2c::DataLen::isPowOfTwo() with a power of two")
{
    CHECK(bt2c::DataLen::fromBits(1).isPowOfTwo());
    CHECK(bt2c::DataLen::fromBits(8).isPowOfTwo());
    CHECK(bt2c::DataLen::fromBits(1024).isPowOfTwo());
}

TEST_CASE("bt2c::DataLen::isPowOfTwo() with a value which isn't a power of two")
{
    CHECK_FALSE(bt2c::DataLen::fromBits(0).isPowOfTwo());
    CHECK_FALSE(bt2c::DataLen::fromBits(9).isPowOfTwo());
    CHECK_FALSE(bt2c::DataLen::fromBits(1023).isPowOfTwo());
}

TEST_CASE("Dereference operator of `bt2c::DataLen` is an alias of bits()")
{
    const auto len = bt2c::DataLen::fromBits(42);

    CHECK(*len == len.bits());
}

TEST_CASE("bt2c::DataLen::operator==() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(8) == bt2c::DataLen::fromBits(8));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) == bt2c::DataLen::fromBits(9));
}

TEST_CASE("bt2c::DataLen::operator!=() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(8) != bt2c::DataLen::fromBits(9));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) != bt2c::DataLen::fromBits(8));
}

TEST_CASE("bt2c::DataLen::operator<() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(8) < bt2c::DataLen::fromBits(9));
    CHECK_FALSE(bt2c::DataLen::fromBits(9) < bt2c::DataLen::fromBits(8));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) < bt2c::DataLen::fromBits(8));
}

TEST_CASE("bt2c::DataLen::operator<=() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(8) <= bt2c::DataLen::fromBits(9));
    CHECK(bt2c::DataLen::fromBits(8) <= bt2c::DataLen::fromBits(8));
    CHECK_FALSE(bt2c::DataLen::fromBits(9) <= bt2c::DataLen::fromBits(8));
}

TEST_CASE("bt2c::DataLen::operator>() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(9) > bt2c::DataLen::fromBits(8));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) > bt2c::DataLen::fromBits(9));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) > bt2c::DataLen::fromBits(8));
}

TEST_CASE("bt2c::DataLen::operator>=() works as expected")
{
    CHECK(bt2c::DataLen::fromBits(9) >= bt2c::DataLen::fromBits(8));
    CHECK(bt2c::DataLen::fromBits(8) >= bt2c::DataLen::fromBits(8));
    CHECK_FALSE(bt2c::DataLen::fromBits(8) >= bt2c::DataLen::fromBits(9));
}

TEST_CASE("bt2c::DataLen::operator+=() adds bits")
{
    auto len = bt2c::DataLen::fromBits(8);

    len += bt2c::DataLen::fromBits(4);
    CHECK(len == bt2c::DataLen::fromBits(12));
}

TEST_CASE("bt2c::DataLen::operator-=() subtracts bits")
{
    auto len = bt2c::DataLen::fromBits(8);

    len -= bt2c::DataLen::fromBits(4);
    CHECK(len == bt2c::DataLen::fromBits(4));
}

TEST_CASE("bt2c::DataLen::operator*=() multiplies bits")
{
    auto len = bt2c::DataLen::fromBits(8);

    len *= 3;
    CHECK(len == bt2c::DataLen::fromBits(24));
}

TEST_CASE("operator+() returns the sum of two `bt2c::DataLen` instances")
{
    CHECK(bt2c::DataLen::fromBits(8) + bt2c::DataLen::fromBits(4) == bt2c::DataLen::fromBits(12));
}

TEST_CASE("operator-() returns the difference of two `bt2c::DataLen` instances")
{
    CHECK(bt2c::DataLen::fromBits(8) - bt2c::DataLen::fromBits(4) == bt2c::DataLen::fromBits(4));
}

TEST_CASE("operator*() returns a multiplied `bt2c::DataLen`")
{
    CHECK(bt2c::DataLen::fromBits(8) * 3 == bt2c::DataLen::fromBits(24));
}

TEST_CASE("`_bits` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(10_bits == bt2c::DataLen::fromBits(10));
}

TEST_CASE("`_KiBits` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_KiBits == bt2c::DataLen::fromBits(2 * 1024ULL));
}

TEST_CASE("`_MiBits` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_MiBits == bt2c::DataLen::fromBits(2 * 1024ULL * 1024));
}

TEST_CASE("`_GiBits` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_GiBits == bt2c::DataLen::fromBits(2 * 1024ULL * 1024 * 1024));
}

TEST_CASE("`_bytes` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(10_bytes == bt2c::DataLen::fromBytes(10));
}

TEST_CASE("`_KiBytes` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_KiBytes == bt2c::DataLen::fromBytes(2 * 1024ULL));
}

TEST_CASE("`_MiBytes` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_MiBytes == bt2c::DataLen::fromBytes(2 * 1024ULL * 1024));
}

TEST_CASE("`_GiBytes` user-defined literal creates the expected `bt2c::DataLen`")
{
    CHECK(2_GiBytes == bt2c::DataLen::fromBytes(2 * 1024ULL * 1024 * 1024));
}
