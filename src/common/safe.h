/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef BABELTRACE_COMMON_SAFE_H
#define BABELTRACE_COMMON_SAFE_H

/*!
@file

@brief
    Safe operations (C).

@ingroup common-c

@code{.c}
#include "common/safe.h"
@endcode

@deprecated
    For new C++ code, use cpp-common/bt2c/safe-ops.hpp.
*/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
@brief
    Returns whether or not \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@retval false
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.
@retval true
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow.
*/
static inline
bool bt_safe_to_mul_int64(int64_t a, int64_t b)
{
	if (a == 0 || b == 0) {
		return true;
	}

	/*
	 * Each division below can't overflow and truncates toward zero,
	 * giving the exact limit for an integral `a` or `b`.
	 */
	if (a > 0) {
		if (b > 0) {
			/* Positive product */
			return a <= INT64_MAX / b;
		}

		/* Negative product */
		return b >= INT64_MIN / a;
	}

	if (b > 0) {
		/* Negative product */
		return a >= INT64_MIN / b;
	}

	/* Positive product (dividing by a negative `b` flips `<=`) */
	return a >= INT64_MAX / b;
}

/*!
@brief
    Returns whether or not \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@retval false
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} overflows.
@retval true
    \bt_p{a}&nbsp;×&nbsp;\bt_p{b} doesn't overflow.
*/
static inline
bool bt_safe_to_mul_uint64(uint64_t a, uint64_t b)
{
	if (a == 0 || b == 0) {
		return true;
	}

	return a < UINT64_MAX / b;
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
static inline
bool bt_safe_to_add_int64(int64_t a, int64_t b)
{
	return a <= INT64_MAX - b;
}

/*!
@brief
    Returns whether or not \bt_p{a}&nbsp;+&nbsp;\bt_p{b} overflows.

@param[in] a
    First term.
@param[in] b
    Second term.

@retval false
    Adding \bt_p{a}&nbsp;+&nbsp;\bt_p{b} overflows.
@retval true
    Adding \bt_p{a}&nbsp;+&nbsp;\bt_p{b} doesn't overflow.
*/
static inline
bool bt_safe_to_add_uint64(uint64_t a, uint64_t b)
{
	return a <= UINT64_MAX - b;
}

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE_COMMON_SAFE_H */
