/*
 * Copyright (c) 2024 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_VERSION_HPP
#define BABELTRACE_CPP_COMMON_BT2_VERSION_HPP

#include "cpp-common/bt2c/c-string-view.hpp"

namespace bt2 {

class Version final
{
public:
    explicit Version(const unsigned int major, const unsigned int minor, const unsigned int patch,
                     const bt2c::CStringView extra) noexcept
        : _mMajor {major},
          _mMinor {minor},
          _mPatch {patch},
          _mExtra {extra}
    {
    }

    unsigned int major() const noexcept
    {
        return _mMajor;
    }

    unsigned int minor() const noexcept
    {
        return _mMinor;
    }

    unsigned int patch() const noexcept
    {
        return _mPatch;
    }

    bt2c::CStringView extra() const noexcept
    {
        return _mExtra;
    }

private:
    unsigned int _mMajor, _mMinor, _mPatch;
    bt2c::CStringView _mExtra;
};

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_VERSION_HPP */
