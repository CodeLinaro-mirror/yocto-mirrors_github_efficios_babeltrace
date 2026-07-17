/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * Babeltrace Trace Converter - Default Configuration
 */

#ifndef BABELTRACE_CLI_BABELTRACE2_CFG_CLI_ARGS_DEFAULT_HPP
#define BABELTRACE_CLI_BABELTRACE2_CFG_CLI_ARGS_DEFAULT_HPP

#include <babeltrace2/babeltrace.h>

#include "babeltrace2-cfg-cli-args.hpp"
#include "babeltrace2-cfg.hpp"

enum bt_config_cli_args_status
bt_config_cli_args_create_with_default(int argc, const char *argv[], struct bt_config **cfg,
                                       const bt_interrupter *interrupter);

#endif /* BABELTRACE_CLI_BABELTRACE2_CFG_CLI_ARGS_DEFAULT_HPP */
