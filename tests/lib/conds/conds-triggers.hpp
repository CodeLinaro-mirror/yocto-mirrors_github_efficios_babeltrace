/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2020-2026 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_TESTS_LIB_CONDS_CONDS_TRIGGERS_HPP
#define BABELTRACE_TESTS_LIB_CONDS_CONDS_TRIGGERS_HPP

#include "utils.hpp"

void addClkClsCompatTriggers(CondTriggers& triggers);
void addClkClsPreCondsTriggers(CondTriggers& triggers);
void addClkSnapshotPreCondsTriggers(CondTriggers& triggers);
void addEventClsPreCondsTriggers(CondTriggers& triggers);
void addEventPreCondsTriggers(CondTriggers& triggers);
void addFcPreCondsTriggers(CondTriggers& triggers);
void addFcTcMatchTriggers(CondTriggers& triggers);
void addFieldLocPreCondsTriggers(CondTriggers& triggers);
void addFieldPathPreCondsTriggers(CondTriggers& triggers);
void addFieldPreCondsTriggers(CondTriggers& triggers);
void addGraphPreCondsTriggers(CondTriggers& triggers);
void addPktPreCondsTriggers(CondTriggers& triggers);
void addStreamClsPreCondsTriggers(CondTriggers& triggers);
void addStreamPreCondsTriggers(CondTriggers& triggers);
void addTraceClassPreCondsTriggers(CondTriggers& triggers);
void addTracePreCondsTriggers(CondTriggers& triggers);

#endif /* BABELTRACE_TESTS_LIB_CONDS_CONDS_TRIGGERS_HPP */
