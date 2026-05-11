/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2020-2026 Philippe Proulx <pproulx@efficios.com>
 */

#include "cpp-common/bt2c/make-span.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

int main(const int argc, const char ** const argv)
{
    CondTriggers triggers;

    addClkClsCompatTriggers(triggers);
    addFcTcMatchTriggers(triggers);
    addClkClsPreCondsTriggers(triggers);
    addClkSnapshotPreCondsTriggers(triggers);
    addEventClsPreCondsTriggers(triggers);
    addEventPreCondsTriggers(triggers);
    addFcPreCondsTriggers(triggers);
    addFieldLocPreCondsTriggers(triggers);
    addFieldPathPreCondsTriggers(triggers);
    addFieldPreCondsTriggers(triggers);
    addGraphPreCondsTriggers(triggers);
    addPktPreCondsTriggers(triggers);
    addStreamClsPreCondsTriggers(triggers);
    addStreamPreCondsTriggers(triggers);
    addTraceClassPreCondsTriggers(triggers);
    addTracePreCondsTriggers(triggers);
    addValueCondsTriggers(triggers);
    condMain(bt2c::makeSpan(argv, argc), triggers);
}
