/*
 * Copyright (c) 2022 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_SAFE_OPS_HPP
#define BABELTRACE_CPP_COMMON_BT2C_SAFE_OPS_HPP

/*!
@file

@brief
    Safe operation template functions.

@ingroup common-cpp-bt2c

@code{.cpp}
#include "cpp-common/bt2c/safe-ops.hpp"
@endcode

This file offers two variants of each operation on integers:

<dl>
  <dt>safeMul(), safeAdd(), and safeSub()
  <dd>
    Returns the result, or no value if the operation overflows, for
    example:

    @code{.cpp}
    if (const auto sum = bt2c::safeAdd(a, b)) {
        // Use `*sum`
    } else {
        // `a + b` overflows
    }
    @endcode

  <dt>safeMulDbg(), safeAddDbg(), and safeSubDbg()
  <dd>
    Asserts that the operation doesn't overflow using BT_ASSERT_DBG(),
    and then returns the result.

    Unless you configured the project with
    <code>BABELTRACE_DEBUG_MODE=1</code>, those functions only perform
    the operation: use them in fast or frequent code paths where an
    overflow is a programming error.
</dl>

The multiplication functions only accept unsigned integers, whereas the
addition and subtraction functions accept both signed and unsigned
integers.
*/

#include <limits>
#include <optional>
#include <type_traits>

#include "common/assert.h"

namespace bt2c {

/*!
@brief
    Returns \bt_p{a}&nbsp;×&nbsp;\bt_p{b}, or no value if it
    overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b}, or no value if
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.
*/
template <typename T>
constexpr std::optional<T> safeMul(const T a, const T b) noexcept
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    if (a != 0 && b != 0 && a > std::numeric_limits<T>::max() / b) {
        return std::nullopt;
    }

    return static_cast<T>(a * b);
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(), and then returns
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b}.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow.
*/
template <typename T>
T safeMulDbg(const T a, const T b) noexcept
{
    BT_ASSERT_DBG(safeMul(a, b));
    return static_cast<T>(a * b);
}

/*!
@brief
    Returns \bt_p{a}&nbsp;+&nbsp;\bt_p{b}, or no value if it
    overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b}, or no value if
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b} overflows.
*/
template <typename T>
constexpr std::optional<T> safeAdd(const T a, const T b) noexcept
{
    static_assert(std::is_integral_v<T>, "`T` is an integral type.");

    if constexpr (std::is_signed_v<T>) {
        if ((b > 0 && a > std::numeric_limits<T>::max() - b) ||
            (b < 0 && a < std::numeric_limits<T>::min() - b)) {
            return std::nullopt;
        }
    } else {
        if (a > std::numeric_limits<T>::max() - b) {
            return std::nullopt;
        }
    }

    return static_cast<T>(a + b);
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(), and then returns
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b}.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow.
*/
template <typename T>
T safeAddDbg(const T a, const T b) noexcept
{
    BT_ASSERT_DBG(safeAdd(a, b));
    return static_cast<T>(a + b);
}

/*!
@brief
    Returns \bt_p{a}&nbsp;−&nbsp;\bt_p{b}, or no value if it
    overflows.

@param[in] a
    Minuend.
@param[in] b
    Subtrahend.

@returns
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b}, or no value if
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b} overflows (with an unsigned
    type: \bt_p{b} is greater than \bt_p{a}).
*/
template <typename T>
constexpr std::optional<T> safeSub(const T a, const T b) noexcept
{
    static_assert(std::is_integral_v<T>, "`T` is an integral type.");

    if constexpr (std::is_signed_v<T>) {
        if ((b < 0 && a > std::numeric_limits<T>::max() + b) ||
            (b > 0 && a < std::numeric_limits<T>::min() + b)) {
            return std::nullopt;
        }
    } else {
        if (a < b) {
            return std::nullopt;
        }
    }

    return static_cast<T>(a - b);
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;−&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(), and then returns
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b}.

@param[in] a
    Minuend.
@param[in] b
    Subtrahend.

@returns
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b} doesn't overflow (with an unsigned
    type: \bt_p{b} isn't greater than \bt_p{a}).
*/
template <typename T>
T safeSubDbg(const T a, const T b) noexcept
{
    BT_ASSERT_DBG(safeSub(a, b));
    return static_cast<T>(a - b);
}

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_SAFE_OPS_HPP */
