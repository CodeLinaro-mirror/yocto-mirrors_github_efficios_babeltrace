/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2011-2013 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 */

#ifndef BABELTRACE_COMPAT_UTC_H
#define BABELTRACE_COMPAT_UTC_H

/*!
@file

@brief
    Wrappers of <code>&lt;%time.h&gt;</code> (POSIX).

@ingroup compat

@code{.c}
#include "compat/utc.h"
@endcode

See \bt_ext_man{time.h,0,0p}.
*/

#include <time.h>

/*!
@brief
    Wrapper of <code>timegm()</code>.

On MinGW, calls <code>_mkgmtime()</code>.

See \bt_ext_man{timegm,3,3}.

@param[in] tm
    See \bt_ext_man{timegm,3,3}.

@returns
    See \bt_ext_man{timegm,3,3}.
*/
static inline
time_t bt_timegm(struct tm *tm)
{
#ifdef __MINGW32__
	return _mkgmtime(tm);
#else
	return timegm(tm);
#endif
}

#endif /* BABELTRACE_COMPAT_UTC_H */
