/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016-2019 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PARAM_PARSE_PARAM_PARSE_HPP
#define BABELTRACE_PARAM_PARSE_PARAM_PARSE_HPP

#include <glib.h>

#include <babeltrace2/babeltrace.h>

/*
 * Converts a `--params` argument to an equivalent map value object.
 *
 * Return value is owned by the caller.
 */
bt_value *bt_param_parse(const char *arg, GString *error);

#endif /* BABELTRACE_PARAM_PARSE_PARAM_PARSE_HPP */
