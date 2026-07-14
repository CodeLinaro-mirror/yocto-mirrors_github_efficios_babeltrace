/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>
#include <cstring>
#include <limits>

#include "cpp-common/bt2c/endian.hpp"

#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"
#include "common.hpp"

TEST_CASE("`bt2c::nativeEndianness` matches the actual host endianness")
{
    const std::uint16_t val = 1;
    unsigned char bytes[sizeof(val)];

    std::memcpy(bytes, &val, sizeof(val));

    if (bytes[0] == 1) {
        CHECK(bt2c::nativeEndianness == bt2c::Endianness::Little);
    } else {
        CHECK(bt2c::nativeEndianness == bt2c::Endianness::Big);
    }
}

TEMPLATE_TEST_CASE(
    "bt2c::littleEndianToNative() and bt2c::bigEndianToNative() with an 8-bit value return it unchanged",
    "[endian]", std::uint8_t, std::int8_t)
{
    const auto val = GENERATE(TestType {0}, TestType {0x12}, std::numeric_limits<TestType>::min(),
                              std::numeric_limits<TestType>::max());

    CHECK(intFromByte(bt2c::littleEndianToNative(val)) == intFromByte(val));
    CHECK(intFromByte(bt2c::bigEndianToNative(val)) == intFromByte(val));
}

TEMPLATE_TEST_CASE_SIG(
    "bt2c::littleEndianToNative() and bt2c::bigEndianToNative() convert an unsigned value according to the host endianness",
    "[endian]", ((typename T, std::uint64_t ValV, std::uint64_t SwappedV), T, ValV, SwappedV),
    (std::uint16_t, 0x0102ULL, 0x0201ULL), (std::uint32_t, 0x01020304ULL, 0x04030201ULL),
    (std::uint64_t, 0x0102030405060708ULL, 0x0807060504030201ULL))
{
    const auto val = static_cast<T>(ValV);
    const auto swapped = static_cast<T>(SwappedV);

    if constexpr (bt2c::nativeEndianness == bt2c::Endianness::Little) {
        CHECK(bt2c::littleEndianToNative(val) == val);
        CHECK(bt2c::bigEndianToNative(val) == swapped);
    } else {
        CHECK(bt2c::bigEndianToNative(val) == val);
        CHECK(bt2c::littleEndianToNative(val) == swapped);
    }
}

TEMPLATE_TEST_CASE_SIG(
    "bt2c::littleEndianToNative() and bt2c::bigEndianToNative() sign-extend the byte-swapped result of a signed value",
    "[endian]", ((typename T, std::int64_t ValV, std::int64_t SwappedV), T, ValV, SwappedV),
    (std::int16_t, 0x00ff, -256), (std::int32_t, 0x000000ff, -16777216),
    (std::int64_t, 0x00000000000000ff, -72057594037927936LL))
{
    const auto val = static_cast<T>(ValV);
    const auto swapped = static_cast<T>(SwappedV);

    if constexpr (bt2c::nativeEndianness == bt2c::Endianness::Little) {
        CHECK(bt2c::littleEndianToNative(val) == val);
        CHECK(bt2c::bigEndianToNative(val) == swapped);
    } else {
        CHECK(bt2c::bigEndianToNative(val) == val);
        CHECK(bt2c::littleEndianToNative(val) == swapped);
    }
}

TEMPLATE_TEST_CASE_SIG(
    "bt2c::littleEndianToNative() and bt2c::bigEndianToNative(), applied twice, return the original value",
    "[endian]", ((typename T, std::uint64_t ValV), T, ValV), (std::uint16_t, 0x0102ULL),
    (std::uint32_t, 0x01020304ULL), (std::uint64_t, 0x0102030405060708ULL))
{
    const auto val = static_cast<T>(ValV);

    CHECK(bt2c::littleEndianToNative(bt2c::littleEndianToNative(val)) == val);
    CHECK(bt2c::bigEndianToNative(bt2c::bigEndianToNative(val)) == val);
}
