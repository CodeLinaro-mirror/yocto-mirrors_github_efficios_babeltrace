/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2016 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 * Copyright 2026 Philippe Proulx <pproulx@efficios.com>
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/common.h"
#include "compat/time.h"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/exc.hpp"

#include "writer.hpp"

namespace bt2pretty {

void Writer::_writeInfoNameEqual(const bt2c::CStringView name)
{
    this->_withColor(_mTermCodes.infoName, [&] {
        this->_appendToBuf(name.data());
    });

    this->_appendToBuf(" = ");
}

void Writer::_writeFieldNameEqual(const bt2c::CStringView name)
{
    this->_withColor(_mTermCodes.fieldName, [&] {
        this->_appendToBuf(name.data());
    });

    this->_appendToBuf(" = ");
}

void Writer::_writeTsCycles(const bt2::ConstClockSnapshot clockSnapshot, const bool updateLast)
{
    const auto cycles = clockSnapshot.value();

    this->_appendFmtToBuf("{:020}", cycles);

    if (updateLast) {
        if (_mLastTs) {
            _mTsDelta = cycles - *_mLastTs;
        }

        _mLastTs = cycles;
    }
}

namespace {

constexpr auto nsPerS = 1'000'000'000LL;

} /* namespace */

void Writer::_writeTsWall(const bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> clkSnapshot,
                          const bool updateLast)
{
    if (!clkSnapshot) {
        this->_appendToBuf("??:??:??.?????????");
        return;
    }

    std::int64_t tsNsec;

    try {
        tsNsec = clkSnapshot->nsFromOrigin();
    } catch (const bt2::OverflowError&) {
        const auto clkCls = clkSnapshot->clockClass();

        BT_CPPLOGW("Overflow computing nanoseconds from origin of clock snapshot: "
                   "clock-class-name=\"{}\", val={}, freq={}",
                   bt2c::maybeNull(clkCls.name().data()), clkSnapshot->value(), clkCls.frequency());
        this->_appendToBuf("Error");
        return;
    }

    if (updateLast) {
        if (_mLastTs) {
            _mTsDelta = static_cast<std::uint64_t>(tsNsec) - *_mLastTs;
        }

        _mLastTs = static_cast<std::uint64_t>(tsNsec);
    }

    std::int64_t tsSec = 0;

    tsSec += tsNsec / nsPerS;
    tsNsec = tsNsec % nsPerS;

    std::uint64_t tsSecAbs;
    std::uint64_t tsNsAbs;
    bool isNeg;

    /*
     * `/` truncates toward zero and `%` carries the sign of the
     * dividend, therefore `tsSec` and `tsNsec` share a sign (with zero
     * compatible with either).
     */
    if (tsSec >= 0 && tsNsec >= 0) {
        isNeg = false;
        tsSecAbs = tsSec;
        tsNsAbs = tsNsec;
    } else {
        BT_ASSERT_DBG(tsSec <= 0);
        BT_ASSERT_DBG(tsNsec <= 0);
        isNeg = true;
        tsSecAbs = -tsSec;
        tsNsAbs = -tsNsec;
    }

    if (this->_tryWriteTsDateTime(tsSecAbs, tsNsAbs, isNeg)) {
        return;
    }

    /* Fall back to seconds from origin */
    this->_appendFmtToBuf("{}{}.{:09}", isNeg ? "-" : "", tsSecAbs, tsNsAbs);
}

bool Writer::_tryWriteTsDateTime(const std::uint64_t tsSecAbs, const std::uint64_t tsNsAbs,
                                 const bool isNeg)
{
    if (_mOpts.clkFmt == ClkFormat::SecFromOrigin) {
        return false;
    }

    if (isNeg && !_mNegTsWarnDone) {
        BT_CPPLOGW("Falling back to seconds from origin format for negative timestamp: "
                   "ts-s={}, ts-ns={}",
                   -static_cast<std::int64_t>(tsSecAbs), tsNsAbs);
        _mNegTsWarnDone = true;
        return false;
    }

    const auto isUtc =
        _mOpts.clkFmt == ClkFormat::UtcTime || _mOpts.clkFmt == ClkFormat::UtcDateTime;

    tm tm;
    const time_t timeS = static_cast<time_t>(tsSecAbs);

    if (const auto res = isUtc ? bt_gmtime_r(&timeS, &tm) : bt_localtime_r(&timeS, &tm); !res) {
        BT_CPPLOGW("Failed to convert seconds to date/time with {}():, ts-s-abs={}",
                   isUtc ? "gmtime" : "localtime", tsSecAbs);
        return false;
    }

    if (_mOpts.clkFmt == ClkFormat::LocalDateTime || _mOpts.clkFmt == ClkFormat::UtcDateTime) {
        char timeStr[26];

        if (const auto res = std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d ", &tm); !res) {
            BT_CPPLOGW("Failed to format date with std::strftime(): ts-s-abs={}", tsSecAbs);
            return false;
        }

        this->_appendToBuf(timeStr);
    }

    /* Write time */
    this->_appendFmtToBuf("{:02}:{:02}:{:02}.{:09}", tm.tm_hour, tm.tm_min, tm.tm_sec, tsNsAbs);
    return true;
}

void Writer::_writeEventMsgTsAndDelta(const bt2::ConstEventMessage eventMsg)
{
    if (!eventMsg.streamClassDefaultClockClass()) {
        /* No default clock class: skip the timestamp without error */
        return;
    }

    /* Timestamp */
    {
        const bt2::ConstClockSnapshot clkSnapshot = eventMsg.defaultClockSnapshot();

        if (_mOpts.writeInfoNames) {
            this->_writeInfoNameEqual("timestamp");
        } else {
            this->_appendToBuf("[");
        }

        this->_withColor(_mTermCodes.ts, [&] {
            if (_mOpts.clkFmt == ClkFormat::Cycles) {
                this->_writeTsCycles(clkSnapshot, true);
            } else {
                this->_writeTsWall(clkSnapshot, true);
            }
        });

        if (!_mOpts.writeInfoNames) {
            this->_appendToBuf("] ");
        }
    }

    /* Timestamp delta */
    if (_mOpts.writeTsDelta) {
        if (_mOpts.writeInfoNames) {
            this->_appendToBuf(", ");
            this->_writeInfoNameEqual("delta");
        } else {
            this->_appendToBuf("(");
        }

        if (_mOpts.clkFmt == ClkFormat::Cycles) {
            if (_mTsDelta) {
                this->_appendFmtToBuf("+{:012}", *_mTsDelta);
            } else {
                /* NOT a trigraph */
                this->_appendToBuf("+??????????\?\?");
            }
        } else {
            if (_mTsDelta) {
                this->_appendFmtToBuf("+{}.{:09}", *_mTsDelta / nsPerS, *_mTsDelta % nsPerS);
            } else {
                this->_appendToBuf("+?.?????????");
            }
        }
        if (!_mOpts.writeInfoNames) {
            this->_appendToBuf(") ");
        }
    }

    _mWroteFirstItem = _mOpts.writeInfoNames;
}

void Writer::_writeEventInfoDomItem(bool& wroteDomItem, const bt2c::CStringView itemName)
{
    if (_mOpts.writeInfoNames) {
        this->_appendItemSep();
        this->_writeInfoNameEqual(itemName);
    } else if (wroteDomItem) {
        this->_appendToBuf(":");
    }

    wroteDomItem = true;
}

void Writer::_writeEventInfoStrEnvDomItem(bool& wroteDomItem, const bt2::ConstTrace trace,
                                          const bool opt, const bt2c::CStringView envKey,
                                          const bt2c::CStringView itemName)
{
    if (!opt) {
        return;
    }

    if (const auto entry = trace.environmentEntry(envKey)) {
        this->_writeEventInfoDomItem(wroteDomItem, itemName);
        this->_appendToBuf(entry->asString().value());
    }
}

void Writer::_writeEventInfo(const bt2::ConstEventMessage eventMsg)
{
    auto wroteDomItem = false;
    const auto event = eventMsg.event();
    const auto eventCls = event.cls();
    const auto trace = event.stream().trace();

    /* Timestamp and delta */
    this->_writeEventMsgTsAndDelta(eventMsg);

    /* Trace name */
    if (_mOpts.writeTraceName) {
        if (const bt2c::CStringView name = trace.name()) {
            if (_mOpts.writeInfoNames) {
                this->_appendItemSep();
                this->_writeInfoNameEqual("trace");
            }

            this->_appendToBuf(name);

            if (!_mOpts.writeInfoNames) {
                this->_appendToBuf(" ");
            }
        }
    }

    /* Hostname (environment) */
    this->_writeEventInfoStrEnvDomItem(wroteDomItem, trace, _mOpts.writeTraceEnvHostname,
                                       "hostname", "trace:hostname");

    /* Domain name (environment) */
    this->_writeEventInfoStrEnvDomItem(wroteDomItem, trace, _mOpts.writeTraceEnvDomainName,
                                       "domain", "trace:domain");

    /* Process name (environment) */
    this->_writeEventInfoStrEnvDomItem(wroteDomItem, trace, _mOpts.writeTraceEnvProcname,
                                       "procname", "trace:procname");

    /* VPID (environment) */
    if (_mOpts.writeTraceEnvVpid) {
        if (const auto vpidVal = trace.environmentEntry("vpid")) {
            this->_writeEventInfoDomItem(wroteDomItem, "trace:vpid");
            this->_appendFmtToBuf("({})", vpidVal->asSignedInteger().value());
        }
    }

    /* Event class log level */
    if (_mOpts.writeEventClsLogLevel) {
        if (const auto logLevel = eventCls.logLevel()) {
            const auto logLevelStr = std::invoke([&]() -> bt2c::CStringView {
                switch (*logLevel) {
                case bt2::EventClassLogLevel::Emergency:
                    return "TRACE_EMERG";

                case bt2::EventClassLogLevel::Alert:
                    return "TRACE_ALERT";

                case bt2::EventClassLogLevel::Critical:
                    return "TRACE_CRIT";

                case bt2::EventClassLogLevel::Error:
                    return "TRACE_ERR";

                case bt2::EventClassLogLevel::Warning:
                    return "TRACE_WARNING";

                case bt2::EventClassLogLevel::Notice:
                    return "TRACE_NOTICE";

                case bt2::EventClassLogLevel::Info:
                    return "TRACE_INFO";

                case bt2::EventClassLogLevel::DebugSystem:
                    return "TRACE_DEBUG_SYSTEM";

                case bt2::EventClassLogLevel::DebugProgram:
                    return "TRACE_DEBUG_PROGRAM";

                case bt2::EventClassLogLevel::DebugProcess:
                    return "TRACE_DEBUG_PROCESS";

                case bt2::EventClassLogLevel::DebugModule:
                    return "TRACE_DEBUG_MODULE";

                case bt2::EventClassLogLevel::DebugUnit:
                    return "TRACE_DEBUG_UNIT";

                case bt2::EventClassLogLevel::DebugFunction:
                    return "TRACE_DEBUG_FUNCTION";

                case bt2::EventClassLogLevel::DebugLine:
                    return "TRACE_DEBUG_LINE";

                case bt2::EventClassLogLevel::Debug:
                    return "TRACE_DEBUG";
                }

                bt_common_abort();
            });

            this->_writeEventInfoDomItem(wroteDomItem, "loglevel");
            this->_appendToBuf(logLevelStr);
            this->_appendFmtToBuf(" ({})", static_cast<int>(*logLevel));
        }
    }

    /* Event class EMF URI */
    if (_mOpts.writeEventClsEmf) {
        if (const auto emfUri = eventCls.emfUri()) {
            this->_writeEventInfoDomItem(wroteDomItem, "model.emf.uri");
            this->_appendToBuf(emfUri);
        }
    }

    if (wroteDomItem && !_mOpts.writeInfoNames) {
        this->_appendToBuf(" ");
    }

    this->_appendItemSep();

    /* End of group */
    _mWroteFirstItem = false;

    /* Event class name */
    {
        if (_mOpts.writeInfoNames) {
            this->_writeInfoNameEqual("name");
        }

        const auto eventClsName = eventCls.name();

        this->_withColor(eventClsName ? _mTermCodes.eventName : _mTermCodes.unknown, [&] {
            if (eventClsName) {
                this->_appendToBuf(eventClsName);
            } else {
                this->_appendToBuf("<unknown>");
            }
        });
    }

    if (_mOpts.writeInfoNames) {
        this->_appendToBuf(", ");
    } else {
        this->_appendToBuf(": ");
    }
}

namespace {

/*
 * Returns `u` with all bits above the lowest `nBits` cleared
 * (handles `nBits == 64` safely).
 */
std::uint64_t maskTo(const std::uint64_t u, const std::uint64_t nBits) noexcept
{
    return u & ((nBits < 64) ? ((UINT64_C(1) << nBits) - 1) : UINT64_MAX);
}

} /* namespace */

void Writer::_writeIntField(const bt2::ConstField field)
{
    const auto fc = field.cls();
    const auto intFc = fc.asInteger();
    const bool isUnsigned = fc.isUnsignedInteger();
    const auto len = intFc.fieldValueRange();
    std::uint64_t u;
    std::int64_t s;

    if (isUnsigned) {
        u = field.asUnsignedInteger().value();
    } else {
        s = field.asSignedInteger().value();
        std::memcpy(&u, &s, sizeof(u));
    }

    this->_withColor(_mTermCodes.numberVal, [&] {
        const auto base = intFc.preferredDisplayBase();

        switch (base) {
        case bt2::DisplayBase::Binary:
            this->_appendFmtToBuf("{:#0{}b}", maskTo(u, len), static_cast<int>(len) + 2);
            break;

        case bt2::DisplayBase::Octal:
            BT_ASSERT_DBG(len != 0);

            /* Round length up to the nearest 3-bit boundary */
            this->_appendFmtToBuf("0{:o}", maskTo(u, ((len - 1) / 3 + 1) * 3));
            break;

        case bt2::DisplayBase::Decimal:
            if (isUnsigned) {
                this->_appendFmtToBuf("{}", u);
            } else {
                this->_appendFmtToBuf("{}", s);
            }

            break;

        case bt2::DisplayBase::Hexadecimal:
            /* Round length up to the nearest nibble */
            this->_appendFmtToBuf("0x{:X}", maskTo(u, (len + 3) & ~UINT64_C(3)));
            break;

        default:
            bt_common_abort();
        }
    });
}

void Writer::_writeEscapedStr(const bt2c::CStringView str)
{
    _mBuf.push_back('"');

    for (const auto ch : str) {
        /* Escape sequences not recognized by iscntrl(). */
        switch (ch) {
        case '\\':
            this->_appendToBuf("\\\\");
            continue;

        case '\'':
            this->_appendToBuf("\\\'");
            continue;

        case '\"':
            this->_appendToBuf("\\\"");
            continue;

        case '\?':
            this->_appendToBuf("\\\?");
            continue;
        }

        /* Standard characters */
        if (!std::iscntrl(static_cast<unsigned char>(ch))) {
            _mBuf.push_back(ch);
            continue;
        }

        switch (ch) {
        case '\a':
            this->_appendToBuf("\\a");
            break;

        case '\b':
            this->_appendToBuf("\\b");
            break;

        case '\e':
            this->_appendToBuf("\\e");
            break;

        case '\f':
            this->_appendToBuf("\\f");
            break;

        case '\n':
            this->_appendToBuf("\\n");
            break;

        case '\r':
            this->_appendToBuf("\\r");
            break;

        case '\t':
            this->_appendToBuf("\\t");
            break;

        case '\v':
            this->_appendToBuf("\\v");
            break;

        default:
            /* Unhandled control sequence: write as hex */
            this->_appendFmtToBuf("\\x{:02x}", static_cast<unsigned char>(ch));
            break;
        }
    }

    _mBuf.push_back('"');
}

void Writer::_writeEnumFieldValLabelUnknown()
{
    this->_withColor(_mTermCodes.unknown, [&] {
        this->_appendToBuf("<unknown>");
    });
}

template <typename LabelArrayT>
void Writer::_writeEnumFieldValLabelArray(const std::uint64_t labelCount,
                                          const LabelArrayT labelArray)
{
    _mSortedLabels.clear();
    _mSortedLabels.reserve(labelCount);

    for (std::uint64_t i = 0; i < labelCount; ++i) {
        _mSortedLabels.push_back(labelArray[i]);
    }

    this->_sortLabels();

    const auto wrap = _mSortedLabels.size() > 1;

    if (wrap) {
        this->_appendToBuf("{ ");
    }

    this->_writeSortedLabels();

    if (wrap) {
        this->_appendToBuf(" }");
    }
}

void Writer::_sortLabels()
{
    std::sort(_mSortedLabels.begin(), _mSortedLabels.end(),
              [](const bt2c::CStringView a, const bt2c::CStringView b) noexcept {
                  return std::strcmp(a, b) < 0;
              });
}

void Writer::_writeSortedLabels()
{
    auto wroteFirst = false;

    for (const auto label : _mSortedLabels) {
        if (wroteFirst) {
            this->_appendToBuf(", ");
        }

        this->_withColor(_mTermCodes.enumMappingName, [&] {
            this->_writeEscapedStr(label);
        });

        wroteFirst = true;
    }
}

void Writer::_writeEnumValBitFlagLabelArrays()
{
    auto wroteFirstLabel = false;

    for (const auto& bitLabels : _mEnumBitLabels) {
        if (const auto labelCount = bitLabels.size(); labelCount > 0) {
            if (wroteFirstLabel) {
                this->_appendToBuf(" | ");
            }

            this->_writeEnumFieldValLabelArray(labelCount, bitLabels.data());
            wroteFirstLabel = true;
        }
    }
}

namespace {

/*
 * Get the labels of the enumeration field class `fc` that map to
 * `value`, appending them to `labels`.
 */
template <typename FcT, typename ValT>
void getEnumLabelsForValue(const FcT fc, const ValT value, std::vector<bt2c::CStringView>& labels)
{
    for (const auto mapping : fc) {
        for (const auto range : mapping.ranges()) {
            const auto lower = range.lower();
            const auto upper = range.upper();

            /*
             * Flag is active if this range represents a single value
             * (lower equal to upper) and the lower value is the same as
             * the bit value to test against.
             */
            if (lower == upper && lower == value) {
                labels.push_back(mapping.label());
                break;
            }
        }
    }
}

} /* namespace */

void Writer::_tryWriteUEnumFieldBitFlags(const bt2::ConstField field)
{
    const auto val = field.asUnsignedInteger().value();

    if (val == 0) {
        /* Value is 0: if there was a label for it, we'd know by now */
        this->_writeEnumFieldValLabelUnknown();
        return;
    }

    const auto fc = field.cls().asUnsignedEnumeration();

    for (std::uint64_t i = 0; i < Writer::_enumMaxBitFlagCount; ++i) {
        const std::uint64_t bitValue = UINT64_C(1) << i;

        if ((val & bitValue) != 0) {
            getEnumLabelsForValue(fc, bitValue, _mEnumBitLabels[i]);

            if (_mEnumBitLabels[i].empty()) {
                /*
                 * This bit has no matching label, therefore this field
                 * is not a bit flag field: write unknown and return.
                 */
                this->_writeEnumFieldValLabelUnknown();
                return;
            }
        }
    }

    this->_writeEnumValBitFlagLabelArrays();
}

void Writer::_tryWriteSEnumFieldBitFlags(const bt2::ConstField field)
{
    const auto val = field.asSignedInteger().value();

    if (val <= 0) {
        /*
         * Negative value: not a bit flag enumeration field.
         *
         * For 0, if there was a value, we'd know by now.
         */
        this->_writeEnumFieldValLabelUnknown();
        return;
    }

    const auto fc = field.cls().asSignedEnumeration();

    for (std::uint64_t i = 0; i < Writer::_enumMaxBitFlagCount; ++i) {
        const std::uint64_t bitValue = UINT64_C(1) << i;

        if ((static_cast<std::uint64_t>(val) & bitValue) != 0) {
            getEnumLabelsForValue(fc, static_cast<std::int64_t>(bitValue), _mEnumBitLabels[i]);

            if (_mEnumBitLabels[i].empty()) {
                this->_writeEnumFieldValLabelUnknown();
                return;
            }
        }
    }

    this->_writeEnumValBitFlagLabelArrays();
}

void Writer::_tryWriteEnumFieldBitFlags(const bt2::ConstField field)
{
    const auto fc = field.cls().asEnumeration();
    const auto intRange = fc.fieldValueRange();

    BT_ASSERT_DBG(intRange <= Writer::_enumMaxBitFlagCount);

    /* Remove all labels from the previous enumeration field */
    for (auto& v : _mEnumBitLabels) {
        v.clear();
    }

    /* Get the mapping labels for the bit value */
    switch (fc.type()) {
    case bt2::FieldClassType::UnsignedEnumeration:
        this->_tryWriteUEnumFieldBitFlags(field);
        break;

    case bt2::FieldClassType::SignedEnumeration:
        this->_tryWriteSEnumFieldBitFlags(field);
        break;

    default:
        bt_common_abort();
    }
}

void Writer::_writeEnumField(const bt2::ConstField field)
{
    const auto labels = std::invoke([field] {
        switch (field.cls().type()) {
        case bt2::FieldClassType::UnsignedEnumeration:
            return field.asUnsignedEnumeration().labels();

        case bt2::FieldClassType::SignedEnumeration:
            return field.asSignedEnumeration().mappingLabels();

        default:
            bt_common_abort();
        }
    });

    this->_appendToBuf("( ");

    if (labels.length() != 0) {
        /* The integral value matches some labels: write them */
        this->_writeEnumFieldValLabelArray(labels.length(), labels);
    } else if (_mOpts.writeEnumFieldFlags) {
        /*
         * The integral value of the enumeration field does _not_ match
         * any label, but the `_mOpts.writeEnumFieldFlags` option is
         * true, so try to decompose the enumeration field value into
         * bits and write it as bit flags.
         */
        this->_tryWriteEnumFieldBitFlags(field);
    } else {
        this->_writeEnumFieldValLabelUnknown();
    }

    /* Write the integral value */
    this->_appendToBuf(" : container = ");
    this->_writeIntField(field);
    this->_appendToBuf(" )");
}

void Writer::_writeBitArrayField(const bt2::ConstField field)
{
    const auto bitArrayField = field.asBitArray();

    this->_withColor(_mTermCodes.numberVal, [&] {
        this->_appendFmtToBuf("0x{:X}", bitArrayField.valueAsInteger());
    });

    const auto labels = bitArrayField.activeFlagLabels();

    if (labels.length() > 0) {
        _mSortedLabels.clear();
        _mSortedLabels.reserve(labels.length());

        for (std::uint64_t i = 0; i < labels.length(); ++i) {
            _mSortedLabels.push_back(labels[i]);
        }

        this->_sortLabels();
        this->_appendToBuf(" { ");
        this->_writeSortedLabels();
        this->_appendToBuf(" }");
    }
}

void Writer::_writeStructFieldMember(const bt2::ConstStructureField field, const std::uint64_t i,
                                     const bool writeNames, std::uint64_t& writtenFieldCount)
{
    if (writtenFieldCount > 0) {
        this->_appendToBuf(", ");
    } else {
        this->_appendToBuf(" ");
    }

    if (writeNames) {
        this->_writeFieldNameEqual(field.cls()[i].name());
    }

    this->_writeField(field[i], writeNames);
    writtenFieldCount += 1;
}

void Writer::_writeStructField(const bt2::ConstField field, const bool writeNames)
{
    this->_appendToBuf("{");

    std::uint64_t nrWrittenFields = 0;
    const auto structField = field.asStructure();
    const auto nrFields = structField.length();

    for (std::uint64_t i = 0; i < nrFields; ++i) {
        this->_writeStructFieldMember(structField, i, writeNames, nrWrittenFields);
    }

    this->_appendToBuf(" }");
}

void Writer::_writeArrayFieldElem(const bt2::ConstArrayField field, const std::uint64_t i,
                                  const bool writeNames)
{
    if (i != 0) {
        this->_appendToBuf(", ");
    } else {
        this->_appendToBuf(" ");
    }

    if (writeNames) {
        this->_appendFmtToBuf("[{}] = ", i);
    }

    this->_writeField(field[i], writeNames);
}

void Writer::_writeArrayField(const bt2::ConstField field, const bool writeNames)
{
    this->_appendToBuf("[");

    const auto arrayField = field.asArray();
    const auto len = arrayField.length();

    for (std::uint64_t i = 0; i < len; ++i) {
        this->_writeArrayFieldElem(arrayField, i, writeNames);
    }

    this->_appendToBuf(" ]");
}

void Writer::_writeOptionField(const bt2::ConstField field, const bool writeNames)
{
    if (const auto innerField = field.asOption().field()) {
        this->_appendToBuf("{ ");

        /*
         * TODO: If `writeNames`, then find selector name using field
         * path/location to print it here.
         */

        this->_writeField(*innerField, writeNames);
        this->_appendToBuf(" }");
    } else {
        this->_appendToBuf("<none>");
    }
}

void Writer::_writeVariantField(const bt2::ConstField field, const bool writeNames)
{
    const auto variantField = field.asVariant();

    this->_appendToBuf("{ ");

    /*
     * TODO: If `writeNames`, then find selector name using field
     * path/location to print it here.
     */

    this->_writeField(variantField.selectedOptionField(), writeNames);
    this->_appendToBuf(" }");
}

void Writer::_writeBlobField(const bt2::ConstField field)
{
    const auto data = field.asBlob().data();

    this->_appendToBuf("{ ");

    for (const auto b : data) {
        this->_appendFmtToBuf("{:02x} ", b);
    }

    this->_appendToBuf("}");
}

void Writer::_writeField(const bt2::ConstField field, const bool writeNames)
{
    switch (field.cls().type()) {
    case bt2::FieldClassType::Bool:
        this->_withColor(_mTermCodes.numberVal, [&] {
            this->_appendToBuf(field.asBool().value() ? "true" : "false");
        });

        break;

    case bt2::FieldClassType::BitArray:
        this->_writeBitArrayField(field);
        break;

    case bt2::FieldClassType::UnsignedInteger:
    case bt2::FieldClassType::SignedInteger:
        this->_writeIntField(field);
        break;

    case bt2::FieldClassType::UnsignedEnumeration:
    case bt2::FieldClassType::SignedEnumeration:
        this->_writeEnumField(field);
        break;

    case bt2::FieldClassType::SinglePrecisionReal:
        this->_withColor(_mTermCodes.numberVal, [&] {
            this->_appendFmtToBuf("{:g}", field.asSinglePrecisionReal().value());
        });

        break;

    case bt2::FieldClassType::DoublePrecisionReal:
        this->_withColor(_mTermCodes.numberVal, [&] {
            this->_appendFmtToBuf("{:g}", field.asDoublePrecisionReal().value());
        });

        break;

    case bt2::FieldClassType::String:
        this->_withColor(_mTermCodes.strVal, [&] {
            this->_writeEscapedStr(field.asString().value());
        });

        break;

    case bt2::FieldClassType::Structure:
        this->_writeStructField(field, writeNames);
        break;

    case bt2::FieldClassType::StaticArray:
    case bt2::FieldClassType::DynamicArrayWithoutLength:
    case bt2::FieldClassType::DynamicArrayWithLength:
        this->_writeArrayField(field, writeNames);
        break;

    case bt2::FieldClassType::OptionWithoutSelector:
    case bt2::FieldClassType::OptionWithBoolSelector:
    case bt2::FieldClassType::OptionWithUnsignedIntegerSelector:
    case bt2::FieldClassType::OptionWithSignedIntegerSelector:
        this->_writeOptionField(field, writeNames);
        break;

    case bt2::FieldClassType::VariantWithoutSelector:
    case bt2::FieldClassType::VariantWithUnsignedIntegerSelector:
    case bt2::FieldClassType::VariantWithSignedIntegerSelector:
        this->_writeVariantField(field, writeNames);
        break;

    case bt2::FieldClassType::StaticBlob:
    case bt2::FieldClassType::DynamicBlobWithoutLengthField:
    case bt2::FieldClassType::DynamicBlobWithLengthField:
        this->_writeBlobField(field);
        break;

    default:
        bt_common_abort();
    }
}

void Writer::_writePktCtxField(const bt2::ConstEvent event)
{
    const auto pkt = event.packet();

    if (!pkt) {
        return;
    }

    const auto field = pkt->contextField();

    if (!field) {
        return;
    }

    this->_appendItemSep();

    if (_mOpts.writeScopeNames) {
        this->_writeInfoNameEqual("stream.packet.context");
    }

    this->_writeField(*field, _mOpts.writeCtxFieldMemberNames);
}

void Writer::_writeCommonEventCtxField(const bt2::ConstEvent event)
{
    const auto field = event.commonContextField();

    if (!field) {
        return;
    }

    this->_appendItemSep();

    if (_mOpts.writeScopeNames) {
        this->_writeInfoNameEqual("stream.event.context");
    }

    this->_writeField(*field, _mOpts.writeCtxFieldMemberNames);
}

void Writer::_writeSpecEventCtxField(const bt2::ConstEvent event)
{
    const auto field = event.specificContextField();

    if (!field) {
        return;
    }

    this->_appendItemSep();

    if (_mOpts.writeScopeNames) {
        this->_writeInfoNameEqual("event.context");
    }

    this->_writeField(*field, _mOpts.writeCtxFieldMemberNames);
}

void Writer::_writeEventPayloadField(const bt2::ConstEvent event)
{
    const auto field = event.payloadField();

    if (!field) {
        return;
    }

    this->_appendItemSep();

    if (_mOpts.writeScopeNames) {
        this->_writeInfoNameEqual("event.fields");
    }

    this->_writeField(*field, _mOpts.writeEventPayloadFieldMemberNames);
}

void Writer::_flushBuf(std::ostream& stream)
{
    if (_mBuf.size() == 0) {
        return;
    }

    stream.write(_mBuf.data(), static_cast<std::streamsize>(_mBuf.size()));

    if (!stream) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Failed to write to output stream.");
    }
}

namespace {

bt2c::CStringView orUnknown(const bt2c::CStringView name) noexcept
{
    return name ? name : "(unknown)";
}

} /* namespace */

void Writer::_writeDiscardedItemsMsgDetails(
    const bt2::ConstStream stream,
    const bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> beginClkSnapshot,
    const bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> endClkSnapshot,
    const std::optional<std::uint64_t> count, const std::string_view elemType)
{
    const auto trace = stream.trace();
    const auto streamName = orUnknown(stream.name());
    const auto traceName = orUnknown(trace.name());

    this->_withColor(_mTermCodes.warnTitle, [&] {
        this->_appendToBuf("WARNING");
    });

    this->_withColor(_mTermCodes.warn, [&] {
        this->_appendFmtToBuf(": {} ", count ? "Tracer discarded" : "Tracer may have discarded");

        if (count) {
            this->_appendFmtToBuf("{} {}{}", *count, elemType, *count == 1 ? "" : "s");
        } else {
            this->_appendFmtToBuf("{}s", elemType);
        }

        this->_appendToBuf(" ");

        if (beginClkSnapshot && endClkSnapshot) {
            const auto writeOneTs = [this](const bt2::ConstClockSnapshot cs) {
                if (_mOpts.clkFmt == ClkFormat::Cycles) {
                    this->_writeTsCycles(cs, false);
                } else {
                    this->_writeTsWall(cs, false);
                }
            };

            this->_appendToBuf("between [");
            writeOneTs(*beginClkSnapshot);
            this->_appendToBuf("] and [");
            writeOneTs(*endClkSnapshot);
            this->_appendToBuf("]");
        } else {
            this->_appendToBuf("(unknown time range)");
        }

        this->_appendFmtToBuf(" in trace \"{}\" ", traceName.str());

        if (_mMipVersion == 0) {
            if (const auto uuid = trace.uuid()) {
                this->_appendFmtToBuf("(UUID: {}) ", uuid->str());
            } else {
                this->_appendToBuf("(no UUID) ");
            }
        } else {
            if (const auto uid = trace.uid()) {
                this->_appendFmtToBuf("(UID: {}) ", uid.str());
            } else {
                this->_appendToBuf("(no UID) ");
            }
        }

        this->_appendFmtToBuf("within stream \"{}\" (stream class ID: {}, stream ID: {}).",
                              streamName.str(), stream.cls().id(), stream.id());
    });

    this->_appendToBuf("\n");

    /*
     * Write to standard error stream, and not as a standard log line,
     * to remain backward compatible with the Babeltrace 1 format.
     */
    this->_flushBuf(std::cerr);
}

void Writer::_writeEventMsg(bt2::ConstEventMessage eventMsg)
{
    const auto event = eventMsg.event();

    this->_writeEventInfo(eventMsg);
    this->_writePktCtxField(event);
    this->_writeCommonEventCtxField(event);
    this->_writeSpecEventCtxField(event);
    this->_writeEventPayloadField(event);
    _mBuf.push_back('\n');
    this->_flushBuf(*_mOut);
}

void Writer::_writeDiscardedItemsMsg(const bt2::ConstMessage msg)
{
    bt2::OptionalBorrowedObject<bt2::ConstStream> stream;
    bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> beginCs;
    bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot> endCs;
    std::optional<std::uint64_t> count;
    std::string_view elemType;

    const auto extractVars = [&](const auto discMsg, const auto hasClkSnapshots, const auto name) {
        stream = discMsg.stream();
        count = discMsg.count();
        elemType = name;

        if (hasClkSnapshots) {
            beginCs = discMsg.beginningDefaultClockSnapshot();
            endCs = discMsg.endDefaultClockSnapshot();
        }
    };

    if (msg.isDiscardedEvents()) {
        const auto discMsg = msg.asDiscardedEvents();

        extractVars(discMsg, discMsg.stream().cls().discardedEventsHaveDefaultClockSnapshots(),
                    "event");
    } else {
        BT_ASSERT(msg.isDiscardedPackets());
        const auto discMsg = msg.asDiscardedPackets();

        extractVars(discMsg, discMsg.stream().cls().discardedPacketsHaveDefaultClockSnapshots(),
                    "packet");
    }

    BT_ASSERT(stream);
    this->_writeDiscardedItemsMsgDetails(*stream, beginCs, endCs, count, elemType);
}

void Writer::writeMsg(const bt2::ConstMessage msg)
{
    _mBuf.clear();
    _mWroteFirstItem = false;

    switch (msg.type()) {
    case bt2::MessageType::Event:
        this->_writeEventMsg(msg.asEvent());
        break;

    case bt2::MessageType::DiscardedEvents:
    case bt2::MessageType::DiscardedPackets:
        this->_writeDiscardedItemsMsg(msg);
        break;

    default:
        break;
    }
}

Writer::Writer(const WriterOpts& opts, std::ostream& out, const std::uint64_t mipVersion,
               const bt2c::Logger& parentLogger)
    : _mOpts {opts},
      _mOut {&out},
      _mMipVersion {mipVersion},
      _mLogger {parentLogger, fmt::format("{}/WRITER", parentLogger.tag())}
{
    const auto& c = _mOpts.colorCodes;

    _mTermCodes.infoName = c.bold;
    _mTermCodes.fieldName = c.fg_cyan;
    _mTermCodes.rst = c.reset;
    _mTermCodes.strVal = c.bold;
    _mTermCodes.numberVal = c.bold;
    _mTermCodes.enumMappingName = c.bold;
    _mTermCodes.unknown = fmt::format("{}{}", c.bold, c.fg_bright_red);
    _mTermCodes.eventName = fmt::format("{}{}", c.bold, c.fg_bright_magenta);
    _mTermCodes.ts = fmt::format("{}{}", c.bold, c.fg_bright_yellow);
    _mTermCodes.warn = c.fg_yellow;
    _mTermCodes.warnTitle = fmt::format("{}{}", c.fg_yellow, c.bold);

    if (_mOpts.writeEnumFieldFlags) {
        /*
         * Allocate all label arrays once and reuse the same set of
         * arrays for all enumerations.
         */
        for (auto& v : _mEnumBitLabels) {
            v.reserve(8);
        }
    }
}

} /* namespace bt2pretty */
