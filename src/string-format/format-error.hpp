/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#ifndef BABELTRACE_STRING_FORMAT_FORMAT_ERROR_HPP
#define BABELTRACE_STRING_FORMAT_FORMAT_ERROR_HPP

#include <string>

#include "common/common.h"
#include "cpp-common/bt2/error.hpp"
#include "cpp-common/bt2c/logging.hpp"

/*
 * Formats an error cause as a string.
 *
 * `columns` is the maximum number of columns to use for folding the error
 *  cause message.
 *
 * `parentLogger` is the logger of the calling context.
 *
 * `useColors` tells whether to include terminal color codes in the output
 * string.
 */
std::string formatBtErrorCause(bt2::ConstErrorCause errorCause, unsigned int columns,
                               const bt2c::Logger& parentLogger, bt_common_color_when useColors);

/*
 * Formats an error as a string.
 *
 * `columns` is the maximum number of columns to use for folding the error
 *  message.
 *
 * `parentLogger` is the logger of the calling context.
 *
 * `useColors` tells whether to include terminal color codes in the output
 * string.
 */
std::string formatBtError(bt2::ConstError error, unsigned int columns,
                          const bt2c::Logger& parentLogger, bt_common_color_when useColors);

#endif /* BABELTRACE_STRING_FORMAT_FORMAT_ERROR_HPP */
