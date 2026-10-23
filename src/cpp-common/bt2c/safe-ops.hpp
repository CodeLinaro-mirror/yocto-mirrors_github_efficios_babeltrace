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
*/

#include <limits>
#include <type_traits>

#include "common/assert.h"

namespace bt2c {

/*!
@brief
    Returns whether or \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@retval false
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.
@retval true
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow.
*/
template <typename T>
constexpr bool safeToMul(const T a, const T b)
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    return a == 0 || b == 0 || a <= std::numeric_limits<T>::max() / b;
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(),
    and then returns \bt_p{a}&nbsp;×&nbsp;\bt_p{b}.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow (safeToMul()
    returns \c true with the same parameters).
*/
template <typename T>
T safeMul(const T a, const T b) noexcept
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    BT_ASSERT_DBG(safeToMul(a, b));
    return a * b;
}

/*!
@brief
    Returns whether or not \bt_p{a}&nbsp;+&nbsp;\bt_p{b} overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@retval false
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b} overflows.
@retval true
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow.
*/
template <typename T>
constexpr bool safeToAdd(const T a, const T b)
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    return a <= std::numeric_limits<T>::max() - b;
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(),
    and then returns \bt_p{a}&nbsp;+&nbsp;\bt_p{b}.

@param[in] a
    First term.
@param[in] b
    Second term.

@returns
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow (safeToAdd()
    returns \c true with the same parameters).
*/
template <typename T>
T safeAdd(const T a, const T b) noexcept
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    BT_ASSERT_DBG(safeToAdd(a, b));
    return a + b;
}

/*!
@brief
    Returns whether or not \bt_p{a}&nbsp;−&nbsp;\bt_p{b} overflows.

@param[in] a
    Minuend.
@param[in] b
    Subtrahend.

@retval false
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b} overflows.
@retval true
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b} doesn't overflow.
*/
template <typename T>
constexpr bool safeToSub(const T a, const T b)
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    return a >= b;
}

/*!
@brief
    Asserts that \bt_p{a}&nbsp;−&nbsp;\bt_p{b} doesn't overflow
    using BT_ASSERT_DBG(),
    and then returns \bt_p{a}&nbsp;−&nbsp;\bt_p{b}.

@param[in] a
    Minuend.
@param[in] b
    Subtrahend.

@returns
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b}

@pre
    \bt_p{a}&nbsp;−&nbsp;\bt_p{b} doesn't overflow (safeToSub()
    returns \c true with the same parameters).
*/
template <typename T>
T safeSub(const T a, const T b) noexcept
{
    static_assert(std::is_unsigned_v<T>, "`T` is an unsigned type.");

    BT_ASSERT_DBG(safeToSub(a, b));
    return a - b;
}

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_SAFE_OPS_HPP */
