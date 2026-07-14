/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2024 EfficiOS Inc.
 */

#ifndef BABELTRACE_TESTS_UTILS_COMMON_HPP
#define BABELTRACE_TESTS_UTILS_COMMON_HPP

#include <cstdint>
#include <ostream>

#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2c/uuid.hpp"

template <typename FuncT>
void forEachMipVersion(FuncT&& func)
{
    for (std::uint64_t v = 0; v <= bt2::getMaximalMipVersion(); ++v) {
        func(v);
    }
}

/* Returns byte `byte` as an `int`. */

inline int intFromByte(const std::uint8_t byte) noexcept
{
    return byte;
}

/* Returns byte `byte` as an `int`. */

inline int intFromByte(const std::int8_t byte) noexcept
{
    return byte;
}

namespace bt2c {

/*
 * Formatter for `bt2c::UuidView`.
 *
 * This is picked up by Catch2 to format a `bt2c::UuidView`.
 */

inline std::ostream& operator<<(std::ostream& os, const UuidView uuid)
{
    os << "UuidView {" << uuid.str() << '}';
    return os;
}

/*
 * Formatter for `bt2c::Uuid`.
 *
 * This is picked up by Catch2 to format a `bt2c::Uuid`.
 */

inline std::ostream& operator<<(std::ostream& os, const Uuid& uuid)
{
    os << "Uuid {" << uuid.str() << '}';
    return os;
}

} /* namespace bt2c */

#endif /* BABELTRACE_TESTS_UTILS_COMMON_HPP */
