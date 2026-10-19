/*
 * Copyright (c) 2022 EfficiOS, inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_GLIB_UP_HPP
#define BABELTRACE_CPP_COMMON_BT2C_GLIB_UP_HPP

/*!
@file

@brief
    GLib unique pointer types.

@ingroup common-cpp-bt2c-up

@code{.cpp}
#include "cpp-common/bt2c/glib-up.hpp"
@endcode
*/

#include <memory>

#include <glib.h>

namespace bt2c {
namespace internal {

struct GCharDeleter final
{
    void operator()(gchar * const p) noexcept
    {
        g_free(p);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/types.html"><code>gchar</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/func.free.html"><code>GLib.free</code></a>.
*/
using GCharUP = std::unique_ptr<gchar, internal::GCharDeleter>;

namespace internal {

struct GStringDeleter final
{
    void operator()(GString * const str)
    {
        g_string_free(str, TRUE);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/struct.String.html"><code>GString</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/method.String.free.html"><code>GLib.String.free</code></a>.
*/
using GStringUP = std::unique_ptr<GString, internal::GStringDeleter>;

namespace internal {

struct GDirDeleter final
{
    void operator()(GDir * const dir)
    {
        g_dir_close(dir);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/struct.Dir.html"><code>GDir</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/method.Dir.close.html"><code>GLib.Dir.close</code></a>.
*/
using GDirUP = std::unique_ptr<GDir, internal::GDirDeleter>;

namespace internal {

struct GMappedFileDeleter final
{
    void operator()(GMappedFile * const f)
    {
        g_mapped_file_unref(f);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/struct.MappedFile.html"><code>GMappedFile</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/method.MappedFile.unref.html"><code>GLib.MappedFile.unref</code></a>.
*/
using GMappedFileUP = std::unique_ptr<GMappedFile, internal::GMappedFileDeleter>;

namespace internal {

struct GTimeZoneDeleter final
{
    void operator()(GTimeZone * const tz) noexcept
    {
        g_time_zone_unref(tz);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/struct.TimeZone.html"><code>GTimeZone</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/method.TimeZone.unref.html"><code>GLib.TimeZone.unref</code></a>.
*/
using GTimeZoneUP = std::unique_ptr<GTimeZone, internal::GTimeZoneDeleter>;

namespace internal {

struct GMatchInfoDeleter final
{
    void operator()(GMatchInfo * const matchInfo) noexcept
    {
        g_match_info_free(matchInfo);
    }
};

} /* namespace internal */

/*!
@brief
    Unique pointer to
    <a href="https://docs.gtk.org/glib/struct.MatchInfo.html"><code>GMatchInfo</code></a>.

The custom deleter calls
<a href="https://docs.gtk.org/glib/method.MatchInfo.free.html"><code>GLib.MatchInfo.free</code></a>.
*/
using GMatchInfoUP = std::unique_ptr<GMatchInfo, internal::GMatchInfoDeleter>;

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_GLIB_UP_HPP */
