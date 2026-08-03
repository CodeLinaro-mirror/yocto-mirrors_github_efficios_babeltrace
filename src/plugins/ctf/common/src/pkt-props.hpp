/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2022 EfficiOS Inc. and Linux Foundation
 */

#ifndef BABELTRACE_PLUGINS_CTF_COMMON_SRC_PKT_PROPS_HPP
#define BABELTRACE_PLUGINS_CTF_COMMON_SRC_PKT_PROPS_HPP

#include <optional>

#include "cpp-common/bt2c/data-len.hpp"
#include "cpp-common/bt2c/logging.hpp"

#include "item-seq/medium.hpp"
#include "metadata/ctf-ir.hpp"

namespace ctf::src {

struct PktProps final
{
    std::optional<bt2c::DataLen> expectedTotalLen;
    std::optional<bt2c::DataLen> expectedContentLen;
    const DataStreamCls *dataStreamCls;
    std::optional<unsigned long long> dataStreamId;

    struct
    {
        std::optional<unsigned long long> discEventRecordCounter;
        std::optional<unsigned long long> beginDefClk;
        std::optional<unsigned long long> endDefClk;
    } snapshots;
};

/*
 * Extract packet properties at offset.
 */
PktProps readPktProps(const TraceCls& traceCls, Medium::UP medium, bt2c::DataLen pktOffset,
                      const bt2c::Logger& parentLogger);

} /* namespace ctf::src */

#endif /* BABELTRACE_PLUGINS_CTF_COMMON_SRC_PKT_PROPS_HPP */
