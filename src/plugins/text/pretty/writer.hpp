/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2016 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 * Copyright 2026 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_TEXT_PRETTY_WRITER_HPP
#define BABELTRACE_PLUGINS_TEXT_PRETTY_WRITER_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/compile.h>
#include <fmt/format.h>

#include "common/common.h"
#include "cpp-common/bt2/message.hpp"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/logging.hpp"

namespace bt2pretty {

enum class ClkFormat
{
    /* Raw cycle count since clock start */
    Cycles,

    /* Decimal seconds since clock origin (`[-]SEC.NS`) */
    SecFromOrigin,

    /* `HH:MM:SS.NSEC` (local time) */
    LocalTime,

    /* `YYYY-MM-DD HH:MM:SS.NSEC` (local time) */
    LocalDateTime,

    /* `HH:MM:SS.NSEC` in (UTC) */
    UtcTime,

    /* `YYYY-MM-DD HH:MM:SS.NSEC` in (UTC) */
    UtcDateTime,
};

struct WriterOpts final
{
    bool writeScopeNames = false;
    bool writeInfoNames = false;
    bool writeCtxFieldMemberNames = false;
    bool writeEventPayloadFieldMemberNames = false;
    bool writeTsDelta = false;
    bool writeEnumFieldFlags = false;
    bool writeEventClsLogLevel = false;
    bool writeEventClsEmf = false;
    bool writeTraceName = false;
    bool writeTraceEnvDomainName = false;
    bool writeTraceEnvProcname = false;
    bool writeTraceEnvVpid = false;
    bool writeTraceEnvHostname = false;
    ClkFormat clkFmt = ClkFormat::LocalTime;

    /*
     * Terminal ANSI color codes to use when writing.
     *
     * Each entry is an empty string when colors are disabled, therefore
     * appending unconditionally is always safe.
     */
    bt_common_color_codes colorCodes = {};
};

class Writer final
{
public:
    /*
     * Builds a pretty-printing writer using `opts` as its options,
     * writing to the `out` stream, assuming a MIP version of
     * `mipVersion`, and using a logger derived from `parentLogger`
     * (same logging level).
     */
    explicit Writer(const WriterOpts& opts, std::ostream& out, std::uint64_t mipVersion,
                    const bt2c::Logger& parentLogger);

    ~Writer() = default;
    Writer(const Writer&) = delete;
    Writer(Writer&&) = delete;
    Writer& operator=(const Writer&) = delete;
    Writer& operator=(Writer&&) = delete;

    /*
     * Writes a pretty-printed representation of `msg` to the
     * output stream.
     *
     * Handles event messages and discarded-events/packets messages;
     * ignores other message types.
     */
    void writeMsg(bt2::ConstMessage msg);

private:
    /*
     * `bt_field_*_enumeration` are backed by 64-bit integers, therefore
     * the maximum number of bit flags in any enumeration field is 64.
     */
    static constexpr std::uint64_t _enumMaxBitFlagCount = 64;

    /*
     * Writes `field` to the output buffer, dispatching to the
     * type-specific writer based on its field class.
     *
     * If `writeNames` is true, prepends each structure/array field
     * member with its name/index.
     */
    void _writeField(bt2::ConstField field, bool writeNames);

    /*
     * Writes `NAME = ` to the output buffer.
     *
     * Use this version for writer-chosen output keys such as
     * `stream.packet.context`, `timestamp`, and `loglevel`.
     */
    void _writeInfoNameEqual(bt2c::CStringView name);

    /*
     * Writes `NAME = ` to the output buffer.
     *
     * Use this version for structure field member names.
     */
    void _writeFieldNameEqual(bt2c::CStringView name);

    /*
     * Writes the raw cycle value of `clkSnapshot` to the output buffer
     * as a zero-padded 20-digit decimal number.
     *
     * If `updateLast` is true, then this function also updates
     * `_mLastTs` and `_mTsDelta` so the next call can produce a delta.
     */
    void _writeTsCycles(bt2::ConstClockSnapshot clkSnapshot, bool updateLast);

    /*
     * Writes a wall-clock representation of `*clkSnapshot` to the
     * output buffer, using the format that `_mOpts.clkFmt` selects.
     *
     * Writes a placeholder if `clkSnapshot` holds no clock snapshot.
     *
     * If `updateLast` is true, then this function also updates
     * `_mLastTs` and `_mTsDelta` so the next call can produce a delta.
     */
    void _writeTsWall(bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> clkSnapshot,
                      bool updateLast);

    /*
     * Tries to write a date/time (depending on `_mOpts.clkFmt`) to the
     * output buffer for `tsSecAbs` and `tsNsAbs` (the absolute parts of
     * a timestamp, with `isNeg` indicating its sign) to the
     * output buffer.
     *
     * Returns false (writing nothing to the output buffer) if
     * `_mOpts.clkFmt` is `ClkFormat::SecFromOrigin`, the timestamp is
     * negative, or one of the underlying timing/format calls fails.
     * Callers should fall back to the `ClkFormat::SecFromOrigin` format
     * in that case.
     */
    bool _tryWriteTsDateTime(std::uint64_t tsSecAbs, std::uint64_t tsNsAbs, bool isNeg);

    /*
     * Writes the timestamp (and optional delta) of `eventMsg` to the
     * output buffer.
     *
     * Updates `_mWroteFirstItem` to reflect whether a previous item was
     * already written in the current group.
     */
    void _writeEventMsgTsAndDelta(bt2::ConstEventMessage eventMsg);

    /*
     * Writes the event "info" header (timestamp, optional trace and
     * environment information, log level, EMF URI, and event name) of
     * `eventMsg` to the output buffer.
     */
    void _writeEventInfo(bt2::ConstEventMessage eventMsg);

    /*
     * Writes the prefix for the next item of the event info "domain
     * chain" (hostname, domain name, process name, VPID, log level, EMF
     * URI), and sets `wroteDomItem` so subsequent items get the proper
     * inter-item separator.
     *
     * The prefix is `, ` plus `<NAME> = ` in info name mode, or `:`
     * between items (with no leading `:` for the first) in
     * compact mode.
     *
     * The caller must append the value of the item to the output buffer
     * immediately after this call.
     */
    void _writeEventInfoDomItem(bool& wroteDomItem, bt2c::CStringView itemName);

    /*
     * If `opt` is true and `trace` has an environment entry named
     * `envKey` holding a string, then this function writes it as the
     * event info domain item `itemName` using _writeEventInfoDomItem().
     */
    void _writeEventInfoStrEnvDomItem(bool& wroteDomItem, bt2::ConstTrace trace, bool opt,
                                      bt2c::CStringView envKey, bt2c::CStringView itemName);

    /*
     * Writes the numeric value of the integer field `field` to the
     * output buffer, using the preferred display base of the class
     * of `field`.
     */
    void _writeIntField(bt2::ConstField field);

    /*
     * Writes `str` to the output buffer between double quotes,
     * escaping control characters and special characters along the
     * way.
     */
    void _writeEscapedStr(bt2c::CStringView str);

    /*
     * Writes the unknown-label marker (`<unknown>`) to the output
     * buffer.
     */
    void _writeEnumFieldValLabelUnknown();

    /*
     * Writes `labelCount` labels from `labelArray` to the output
     * buffer in alphabetical order, separating multiple labels with `,`
     * and wrapping them between `{` and `}`.
     */
    template <typename LabelArrayT>
    void _writeEnumFieldValLabelArray(std::uint64_t labelCount, LabelArrayT labelArray);

    /*
     * Sorts `_mSortedLabels` alphabetically (using std::strcmp()).
     */
    void _sortLabels();

    /*
     * Writes the contents of `_mSortedLabels` to the output buffer,
     * coloring each label with `_mTermCodes.enumMappingName` and
     * separating consecutive labels with `, `.
     */
    void _writeSortedLabels();

    /*
     * Writes the per-bit labels that _tryWriteUEnumFieldBitFlags() or
     * _tryWriteSEnumFieldBitFlags() gathered into `_mEnumBitLabels` to
     * the output buffer, joining the bit groups with ` | `.
     */
    void _writeEnumValBitFlagLabelArrays();

    /*
     * Splits the unsigned enumeration field `field` into its set bits
     * and, when every set bit matches a label, writes them as ORed
     * bit flags.
     *
     * Falls back to writing the unknown label marker otherwise.
     */
    void _tryWriteUEnumFieldBitFlags(bt2::ConstField field);

    /*
     * Splits the signed enumeration field `field` into its set bits
     * and, when every set bit matches a label, writes them as ORed
     * bit flags.
     *
     * Falls back to writing the unknown label marker otherwise.
     */
    void _tryWriteSEnumFieldBitFlags(bt2::ConstField field);

    /*
     * Dispatches to either _tryWriteUEnumFieldBitFlags() or
     * _tryWriteSEnumFieldBitFlags() based on the signedness of the
     * enumeration field `field`.
     */
    void _tryWriteEnumFieldBitFlags(bt2::ConstField field);

    /*
     * Writes the enumeration field `field` to the output buffer,
     * including its label(s) (or, when enabled, the bit flag
     * decomposition) and the integer value.
     */
    void _writeEnumField(bt2::ConstField field);

    /*
     * Writes the bit array field `field` to the output buffer as a
     * hexadecimal value, and then writes the sorted list of active flag
     * labels (if any) wrapped between `{` and `}`.
     */
    void _writeBitArrayField(bt2::ConstField field);

    /*
     * Writes the member at index `i` of `structField` to the output
     * buffer, prepending a comma separator when `nrWrittenFields` is
     * greater than zero, and then increments `nrWrittenFields`.
     *
     * If `writeNames` is true, prepends the value with the name of
     * the member.
     */
    void _writeStructFieldMember(bt2::ConstStructureField structField, std::uint64_t i,
                                 bool writeNames, std::uint64_t& nrWrittenFields);

    /*
     * Writes the structure `structField` to the output buffer,
     * including the enclosing `{`/`}`.
     */
    void _writeStructField(bt2::ConstField structField, bool writeNames);

    /*
     * Writes the element at index `i` of `field` to the output buffer,
     * prepending a comma separator when `i` is greater than zero.
     *
     * If `writeNames` is true, prepends the value with `[INDEX] = `.
     */
    void _writeArrayFieldElem(bt2::ConstArrayField field, std::uint64_t i, bool writeNames);

    /*
     * Writes the (static or dynamic) array field `field` to the output
     * buffer, including the enclosing `[`/`]`.
     */
    void _writeArrayField(bt2::ConstField field, bool writeNames);

    /*
     * Writes the option `field` to the output buffer: either its
     * contained field wrapped between `{` and `}`, or `<none>` when the
     * option holds no value.
     */
    void _writeOptionField(bt2::ConstField field, bool writeNames);

    /*
     * Writes the variant `field` to the output buffer: the field of its
     * selected option, wrapped between `{` and `}`.
     */
    void _writeVariantField(bt2::ConstField field, bool writeNames);

    /*
     * Writes the BLOB `field` to the output buffer as a space-separated
     * list of hexadecimal bytes wrapped between `{` and `}`.
     */
    void _writeBlobField(bt2::ConstField field);

    /*
     * Writes the packet context field of `event`, if any, to the output
     * buffer, prepending the `stream.packet.context` scope name when
     * `_mOpts.writeScopeNames` is true.
     */
    void _writePktCtxField(bt2::ConstEvent event);

    /*
     * Writes the common event context field of `event`, if any, to the
     * output buffer, prepending the `stream.event.context` scope name
     * when `_mOpts.writeScopeNames` is true.
     */
    void _writeCommonEventCtxField(bt2::ConstEvent event);

    /*
     * Writes the specific context field of `event`, if any, to the
     * output buffer, prepending the `event.context` scope name when
     * `_mOpts.writeScopeNames` is true.
     */
    void _writeSpecEventCtxField(bt2::ConstEvent event);

    /*
     * Writes the payload field of `event`, if any, to the output
     * buffer, prepending the `event.fields` scope name when
     * `_mOpts.writeScopeNames` is true.
     */
    void _writeEventPayloadField(bt2::ConstEvent event);

    /*
     * Writes a single "tracer discarded N events/packets" warning line
     * to the standard error stream, describing `stream`, the
     * beginning/end clock snapshots (when both are available), the
     * discarded `count` (or uses "may have discarded" when `count` has
     * no value), and `elemType` (`event` or `packet`).
     */
    void _writeDiscardedItemsMsgDetails(
        bt2::ConstStream stream,
        bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> beginClkSnapshot,
        bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> endClkSnapshot,
        std::optional<std::uint64_t> count, std::string_view elemType);

    /*
     * Writes the full event message `eventMsg` (info, context
     * fields, and payload field) to the output stream.
     */
    void _writeEventMsg(bt2::ConstEventMessage eventMsg);

    /*
     * Extracts the stream, clock snapshots, and count from the
     * discarded events/packets message `msg`, and then calls
     * _writeDiscardedItemsMsgDetails() to write the warning.
     */
    void _writeDiscardedItemsMsg(bt2::ConstMessage msg);

    /*
     * Appends `str` to the output buffer.
     */
    void _appendToBuf(const std::string_view str)
    {
        _mBuf.append(str.data(), str.data() + str.size());
    }

    /*
     * Appends the terminal escape sequence `termCode` to the output
     * buffer, calls `func()`, and then appends the reset escape sequence.
     */
    template <typename FuncT>
    void _withColor(const std::string_view termCode, FuncT&& func)
    {
        this->_appendToBuf(termCode);
        func();
        this->_appendToBuf(_mTermCodes.rst);
    }

    /*
     * Formats `args` according to `fmtStr` and appends the result to
     * the output buffer.
     *
     * `fmtStr` may be either a regular `fmt::format_string` or, on hot
     * paths, an `FMT_COMPILE(...)`-wrapped string so the format spec is
     * parsed at compile time.
     */
    template <typename FmtT, typename... ArgsT>
    void _appendFmtToBuf(const FmtT& fmtStr, ArgsT&&...args)
    {
        fmt::format_to(std::back_inserter(_mBuf), fmtStr, std::forward<ArgsT>(args)...);
    }

    /*
     * Appends `, ` to the output buffer to separate the next item from
     * the previous one in the current group, if a prior item was already
     * written, then marks `_mWroteFirstItem` so subsequent calls get the
     * separator.
     */
    void _appendItemSep()
    {
        if (_mWroteFirstItem) {
            this->_appendToBuf(", ");
        }

        _mWroteFirstItem = true;
    }

    /*
     * Writes the contents of the output buffer to `stream` and clears
     * the buffer. Throws `bt2c::Error` on write failure.
     */
    void _flushBuf(std::ostream& stream);

    /* Pretty-printing options */
    WriterOpts _mOpts;

    /* Output stream */
    std::ostream *_mOut;

    /* Effective MIP version to consider */
    std::uint64_t _mMipVersion;

    /* Logger */
    bt2c::Logger _mLogger;

    /*
     * Whether at least one top-level item has already been written in
     * the current group, meaning that a leading `, ` separator should
     * be emitted before the next one.
     *
     * Reset to false at the start of each message write.
     */
    bool _mWroteFirstItem = false;

    /* Output buffer; _flushBuf() empties this into `_mOut` */
    fmt::memory_buffer _mBuf;

    /*
     * Last timestamp, if any.
     *
     * Cycles when `_mOpts.clkFmt` is `ClkFormat::Cycles`, otherwise
     * nanoseconds from origin.
     */
    std::optional<std::uint64_t> _mLastTs;

    /* Delta between the last two timestamps, if any */
    std::optional<std::uint64_t> _mTsDelta;

    /*
     * Whether _writeTsWall() already emitted the one-shot
     * negative timestamp warning.
     */
    bool _mNegTsWarnDone = false;

    /*
     * For each bit of the integer backing the enumeration field value:
     * the labels for that bit.
     *
     * The vectors are allocated once during construction and reused
     * across enumeration fields to avoid
     * repeated allocation/deallocation.
     */
    std::array<std::vector<bt2c::CStringView>, _enumMaxBitFlagCount> _mEnumBitLabels;

    /*
     * Reusable, alphabetically ordered list of labels.
     *
     * Cleared and refilled before each label group is written; the
     * underlying buffer is retained across groups so subsequent fills
     * avoid reallocation.
     */
    std::vector<bt2c::CStringView> _mSortedLabels;

    /*
     * Terminal escape sequences used to colorize the output.
     *
     * Populated by the constructor from `_mOpts.colorCodes`: each entry
     * is empty when colors are disabled. Callers append the relevant
     * entries unconditionally, since appending an empty string is
     * a no-op.
     */
    struct
    {
        std::string infoName;
        std::string fieldName;
        std::string rst;
        std::string strVal;
        std::string numberVal;
        std::string enumMappingName;
        std::string unknown;
        std::string eventName;
        std::string ts;
        std::string warn;
        std::string warnTitle;
    } _mTermCodes;
};

} /* namespace bt2pretty */

#endif /* BABELTRACE_PLUGINS_TEXT_PRETTY_WRITER_HPP */
