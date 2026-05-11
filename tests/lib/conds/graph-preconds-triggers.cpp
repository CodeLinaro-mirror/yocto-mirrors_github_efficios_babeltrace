/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include "cpp-common/bt2/graph.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds graph API precondition failure triggers.
 */
void addGraphPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt2::Graph::create(292);
        },
        "graph-create:valid-mip-version");
}
