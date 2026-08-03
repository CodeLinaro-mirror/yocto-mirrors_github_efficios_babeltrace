/*
 * Copyright (c) 2022 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include "common/assert.h"

#include "json/ctf-2-metadata-stream-parser.hpp"
#include "metadata-stream-parser-utils.hpp"
#include "tsdl/ctf-1-metadata-stream-parser.hpp"

namespace ctf::src {

MetadataStreamMajorVersion getMetadataStreamMajorVersion(const bt2c::ConstBytes buffer) noexcept
{
    {
        BT_ASSERT(buffer.data());

        /* CTF 2 if it starts with an RS byte, otherwise CTF 1 */
        return (buffer[0] == 30) ? MetadataStreamMajorVersion::V2 : MetadataStreamMajorVersion::V1;
    }
}

std::unique_ptr<MetadataStreamParser>
createMetadataStreamParser(const MetadataStreamMajorVersion majorVersion,
                           const bt2::OptionalBorrowedObject<bt2::SelfComponent> selfComp,
                           const ClkClsCfg& clkClsCfg, const bt2c::Logger& parentLogger)
{
    if (majorVersion == MetadataStreamMajorVersion::V1) {
        return std::make_unique<Ctf1MetadataStreamParser>(selfComp, clkClsCfg, parentLogger);
    } else {
        BT_ASSERT(majorVersion == MetadataStreamMajorVersion::V2);
        return std::make_unique<Ctf2MetadataStreamParser>(selfComp, clkClsCfg, parentLogger);
    }
}

std::unique_ptr<MetadataStreamParser>
createMetadataStreamParser(const bt2c::ConstBytes buffer,
                           const bt2::OptionalBorrowedObject<bt2::SelfComponent> selfComp,
                           const ClkClsCfg& clkClsCfg, const bt2c::Logger& parentLogger)
{
    return createMetadataStreamParser(getMetadataStreamMajorVersion(buffer), selfComp, clkClsCfg,
                                      parentLogger);
}

MetadataStreamParser::ParseRet
parseMetadataStream(const bt2::OptionalBorrowedObject<bt2::SelfComponent> selfComp,
                    const ClkClsCfg& clkClsCfg, const bt2c::ConstBytes buffer,
                    const bt2c::Logger& parentLogger)
{
    if (const auto majorVersion = getMetadataStreamMajorVersion(buffer);
        majorVersion == MetadataStreamMajorVersion::V1) {
        return Ctf1MetadataStreamParser::parse(selfComp, clkClsCfg, buffer, parentLogger);
    } else {
        BT_ASSERT(majorVersion == MetadataStreamMajorVersion::V2);
        return Ctf2MetadataStreamParser::parse(selfComp, clkClsCfg, buffer, parentLogger);
    }
}

} /* namespace ctf::src */
