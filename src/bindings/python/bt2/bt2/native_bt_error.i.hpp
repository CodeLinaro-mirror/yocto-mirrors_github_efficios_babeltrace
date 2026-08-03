/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2019 Efficios, Inc.
 */

#ifndef BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_ERROR_I_HPP
#define BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_ERROR_I_HPP

#include <string-format/format-error.hpp>

static PyObject *bt_bt2_format_bt_error_cause(const bt_error_cause *error_cause)
{
    PyObject *py_error_cause_str = NULL;
    const auto errorCauseStr = formatBtErrorCause(
        bt2::ConstErrorCause {error_cause}, 80,
        bt2c::Logger("bt2_format_bt_error_cause () (Python)", BT_LOG_TAG,
                     static_cast<bt2c::Logger::Level>(bt_python_bindings_bt2_log_level)),
        BT_COMMON_COLOR_WHEN_NEVER);

    py_error_cause_str = PyUnicode_FromString(errorCauseStr.c_str());

    return py_error_cause_str;
}

static PyObject *bt_bt2_format_bt_error(const bt_error *error)
{
    PyObject *py_error_str = NULL;
    const auto errorStr = formatBtError(
        bt2::ConstError {error}, 80,
        bt2c::Logger("bt2_format_bt_error () (Python)", BT_LOG_TAG,
                     static_cast<bt2c::Logger::Level>(bt_python_bindings_bt2_log_level)),
        BT_COMMON_COLOR_WHEN_NEVER);

    py_error_str = PyUnicode_FromString(errorStr.c_str());

    return py_error_str;
}

#endif /* BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_ERROR_I_HPP */
