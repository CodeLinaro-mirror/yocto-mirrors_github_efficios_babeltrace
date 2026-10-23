/*
 * Copyright (c) 2024 EfficiOS Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_REGEX_HPP
#define BABELTRACE_CPP_COMMON_BT2C_REGEX_HPP

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <glib.h>

#include "common/assert.h"
#include "cpp-common/bt2c/glib-up.hpp"
#include "cpp-common/bt2c/logging.hpp"

namespace bt2c {

/*!
@brief
    Successful match of a regex, with the substrings which its
    capturing groups match.

@ingroup common-cpp-bt2c

Get an instance with Regex::matchGroups().

Group&nbsp;0 is the whole match, while group&nbsp;<em>N</em>
(<em>N</em>&nbsp;≥&nbsp;1) is the <em>N</em>th capturing group
of the pattern.

The substrings are views on the string which you pass to
Regex::matchGroups(): an instance remains valid as long as said
string isn't modified.

@code{.cpp}
#include "cpp-common/bt2c/regex.hpp"
@endcode
*/
class RegexMatch final
{
    friend class Regex;

private:
    explicit RegexMatch(std::vector<std::optional<std::string_view>> groups) noexcept
        : _mGroups {std::move(groups)}
    {
    }

public:
    /*!
    @brief
        Number of groups, including group&nbsp;0 (whole match).

    This is the number of capturing groups of the pattern
    plus one, whether or not they participate in the match.

    @returns
        Number of groups.
    */
    std::size_t count() const noexcept
    {
        return _mGroups.size();
    }

    /*!
    @brief
        Substring which the group at the index
        \bt_p{index} matches.

    @param[in] index
        Index of the group to get (0 for the whole match).

    @pre
        \bt_p{index} is less than count().

    @returns
        Substring which the group at the index \bt_p{index}
        matches (possibly empty), or \c std::nullopt if this group
        doesn't participate in the match, for example an optional
        group or a non-taken alternative.
    */
    std::optional<std::string_view> operator[](const std::size_t index) const noexcept
    {
        BT_ASSERT_DBG(index < _mGroups.size());
        return _mGroups[index];
    }

private:
    std::vector<std::optional<std::string_view>> _mGroups;
};

/*!
@brief
    Regular expression.

@ingroup common-cpp-bt2c

This is a simple regex class of which an instance compiles a
single pattern.

The regex language is currently the one of
<a href="https://docs.gtk.org/glib/struct.Regex.html"><code>GLib.Regex</code></a>
which is similar to
<a href="https://www.pcre.org/">PCRE</a>.

Match a string with match(), or with matchGroups() to also get the
substrings which the capturing groups of the pattern match.

@code{.cpp}
#include "cpp-common/bt2c/regex.hpp"
@endcode
*/
class Regex final
{
public:
    /*!
    @brief
        Builds a regex object, compiling the pattern \bt_p{pattern}.

    @param[in] pattern
        %Regex pattern to compile.

    @pre
        \bt_p{pattern} is a valid
        <a href="https://docs.gtk.org/glib/struct.Regex.html"><code>GLib.Regex</code></a>
        regex.
    */
    explicit Regex(const char * const pattern) noexcept
    {
        GError *error = nullptr;

        _mRegex = g_regex_new(pattern, G_REGEX_OPTIMIZE, static_cast<GRegexMatchFlags>(0), &error);

        if (!_mRegex) {
            BT_CPPLOGF_SPEC((bt2c::Logger {"BT2C", "REGEX", bt2c::Logger::Level::Fatal}),
                            "g_regex_new() failed: {}", error->message);
            bt_common_abort();
        }
    }

    Regex(const Regex&) = delete;
    Regex& operator=(const Regex&) = delete;

    ~Regex()
    {
        g_regex_unref(_mRegex);
    }

    /*!
    @brief
        Returns whether or not this regex
        matches \bt_p{str}.

    See
    <a href="https://docs.gtk.org/glib/method.Regex.match_full.html"><code>GLib.Regex.match_full</code></a>.

    @param[in] str
        String to match.

    @retval false
        This regex doesn't match \bt_p{str}.
    @retval true
        This regex matches \bt_p{str}.

    @sa matchGroups()
    */
    bool match(const std::string_view str) const noexcept
    {
        return g_regex_match_full(_mRegex, _strData(str), str.size(), 0,
                                  static_cast<GRegexMatchFlags>(0), nullptr, nullptr);
    }

    /*!
    @brief
        Returns the match of this regex on \bt_p{str}, if any,
        with the substrings which its capturing groups match.

    See
    <a href="https://docs.gtk.org/glib/method.Regex.match_full.html"><code>GLib.Regex.match_full</code></a>
    and
    <a href="https://docs.gtk.org/glib/method.MatchInfo.fetch_pos.html"><code>GLib.MatchInfo.fetch_pos</code></a>.

    @param[in] str
        String to match.

    @returns
        Match of this regex on \bt_p{str}, or \c std::nullopt if this
        regex doesn't match \bt_p{str}.

        The returned match contains views on \bt_p{str}.

    @sa match()
    */
    std::optional<RegexMatch> matchGroups(const std::string_view str) const
    {
        GMatchInfo *rawMatchInfo = nullptr;

        /* g_regex_match_full() sets `rawMatchInfo` even on mismatch */
        const auto matches =
            g_regex_match_full(_mRegex, _strData(str), str.size(), 0,
                               static_cast<GRegexMatchFlags>(0), &rawMatchInfo, nullptr);
        const GMatchInfoUP matchInfo {rawMatchInfo};

        if (!matches) {
            return std::nullopt;
        }

        /*
         * Use the capture count of the pattern rather than
         * g_match_info_get_match_count() which excludes trailing groups
         * which don't participate in the match.
         */
        std::vector<std::optional<std::string_view>> groups(
            static_cast<std::size_t>(g_regex_get_capture_count(_mRegex)) + 1);

        for (std::size_t i = 0; i < groups.size(); ++i) {
            gint startPos, endPos;

            /*
             * g_match_info_fetch_pos() either returns `FALSE` or sets
             * `startPos` to -1 when the group doesn't participate in
             * the match.
             */
            if (g_match_info_fetch_pos(matchInfo.get(), static_cast<gint>(i), &startPos, &endPos) &&
                startPos >= 0) {
                groups[i] = str.substr(static_cast<std::size_t>(startPos),
                                       static_cast<std::size_t>(endPos - startPos));
            }
        }

        return RegexMatch {std::move(groups)};
    }

private:
    /*
     * Returns the data of `str`, or an empty string if it's `nullptr`
     * (default-constructed string view), as GLib requires a
     * non-null string.
     */
    static const char *_strData(const std::string_view str) noexcept
    {
        return str.data() ? str.data() : "";
    }

    GRegex *_mRegex;
};

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_REGEX_HPP */
