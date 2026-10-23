/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2019-2026 Philippe Proulx <pproulx@efficios.com>
 */

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <fmt/core.h>
#include <glib.h>

#include <babeltrace2/babeltrace.h>

#include "common/assert.h"
#include "common/common.h"
#include "cpp-common/bt2/clock-snapshot.hpp"
#include "cpp-common/bt2c/fmt.hpp" /* IWYU pragma: keep */

#include "comp.hpp"
#include "msg-iter.hpp"

namespace bt2trimmer {

MsgIter::MsgIter(const bt2::SelfMessageIterator selfMsgIter,
                 const bt2::SelfMessageIteratorConfiguration config, bt2::SelfComponentOutputPort)
    : bt2::UserMessageIterator<MsgIter, Comp> {selfMsgIter, "MSG-ITER"},
      _mUpstreamMsgIter {
          this->_createMessageIterator(this->_component()._inputPorts()[Comp::_inPortName])},
      _mBegin {this->_component()._mBegin},
      _mEnd {this->_component()._mEnd}
{
    /*
     * If both bounds of the trimming time range are already full times
     * or infinite, then skip the `_State::SetBoundsNsFromOrig` state.
     */
    _mState = _mBegin.needsDate() || _mEnd.needsDate() ? _State::SetBoundsNsFromOrig :
                                                         _State::SeekInitially;

    /*
     * The trimmer requires upstream messages to have times, so it can
     * always seek forward.
     */
    config.canSeekForward(true);
}

void MsgIter::_next(bt2::ConstMessageArray& msgs)
{
    switch (_mState) {
    case _State::SetBoundsNsFromOrig:
        if (!this->_setBoundsNsFromOrig()) {
            /* Upstream message iterator ended */
            _mState = _State::Ended;
            return;
        }

        _mState = _State::SeekInitially;
        [[fallthrough]];

    case _State::SeekInitially:
        this->_seekInitially();
        _mState = _State::Trim;
        [[fallthrough]];

    case _State::Trim:
        this->_trim(msgs);
        break;

    case _State::Ending:
        this->_ending(msgs);
        break;

    case _State::Ended:
        break;
    }
}

bool MsgIter::_setBoundsNsFromOrig()
{
    BT_ASSERT(_mBegin.needsDate() || _mEnd.needsDate());

    /* Find the time of the first message having one */
    const auto nsFromOrig = std::invoke([this]() -> std::optional<std::int64_t> {
        while (true) {
            const auto msgs = _mUpstreamMsgIter->next();

            if (!msgs) {
                /* Ended */
                return std::nullopt;
            }

            for (const auto msg : *msgs) {
                if (const auto msgNsFromOrig = this->_msgNsFromOrig(msg)) {
                    BT_ASSERT_DBG(*msgNsFromOrig != INT64_MIN && *msgNsFromOrig != INT64_MAX);
                    return msgNsFromOrig;
                }
            }
        }
    });

    if (!nsFromOrig) {
        return false;
    }

    /* Set the date of the time of day bounds */
    auto& comp = this->_component();

    if (_mBegin.needsDate()) {
        _mBegin = comp._fullBound(_mBegin.timeOfDay(), *nsFromOrig);
    }

    if (_mEnd.needsDate()) {
        _mEnd = comp._fullBound(_mEnd.timeOfDay(), *nsFromOrig);
    }

    comp._validateBounds(_mBegin, _mEnd);
    return true;
}

void MsgIter::_seekInitially()
{
    BT_ASSERT(!_mBegin.needsDate());

    if (_mBegin.isInfinite()) {
        bool canSeek;

        try {
            canSeek = _mUpstreamMsgIter->canSeekBeginning();
        } catch (const bt2c::Error&) {
            BT_CPPLOGE_APPEND_CAUSE_AND_RETHROW(
                "Cannot make upstream message iterator initially seek its beginning.");
        }

        if (!canSeek) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "Cannot make upstream message iterator initially seek its beginning.");
        }

        _mUpstreamMsgIter->seekBeginning();
    } else {
        bool canSeek;

        try {
            canSeek = _mUpstreamMsgIter->canSeekNsFromOrigin(_mBegin.nsFromOrig());
        } catch (const bt2c::Error&) {
            BT_CPPLOGE_APPEND_CAUSE_AND_RETHROW(
                "Cannot make upstream message iterator initially seek: seek-ns-from-origin={}",
                _mBegin.nsFromOrig());
        }

        if (!canSeek) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error,
                "Cannot make upstream message iterator initially seek: seek-ns-from-origin={}",
                _mBegin.nsFromOrig());
        }

        _mUpstreamMsgIter->seekNsFromOrigin(_mBegin.nsFromOrig());
    }
}

void MsgIter::_trim(bt2::ConstMessageArray& msgs)
{
    while (_mOutputMsgs.empty()) {
        const auto upstreamMsgs = _mUpstreamMsgIter->next();

        if (G_UNLIKELY(!upstreamMsgs)) {
            /* Upstream message iterator ended */
            this->_endStreams();
            _mState = _State::Ending;
            this->_ending(msgs);
            return;
        }

        BT_ASSERT_DBG(!upstreamMsgs->isEmpty());

        for (const auto msg : *upstreamMsgs) {
            if (G_UNLIKELY(this->_handleMsg(msg))) {
                /*
                 * The time of this message is past the end time of the
                 * trimming time range: we're done.
                 *
                 * There might still be messages in the output message
                 * queue, so move to the "ending" state and apply
                 * it immediately.
                 */
                _mState = _State::Ending;
                this->_ending(msgs);
                return;
            }
        }
    }

    /*
     * There's at least one message in the output message queue: move
     * the messages to the output message array.
     */
    this->_moveOutputMsgs(msgs);
}

void MsgIter::_ending(bt2::ConstMessageArray& msgs)
{
    if (_mOutputMsgs.empty()) {
        _mState = _State::Ended;
        return;
    }

    this->_moveOutputMsgs(msgs);
}

void MsgIter::_moveOutputMsgs(bt2::ConstMessageArray& msgs)
{
    BT_ASSERT_DBG(!_mOutputMsgs.empty());

    while (!msgs.isFull() && !_mOutputMsgs.empty()) {
        msgs.append(std::move(_mOutputMsgs.front()));
        _mOutputMsgs.pop_front();
    }
}

std::optional<std::int64_t> MsgIter::_msgNsFromOrig(const bt2::ConstMessage msg) const
{
    /* Throws if `msg` doesn't have a default clock class */
    const auto validateHasClkCls = [this, msg](const bool hasClkCls) {
        if (G_UNLIKELY(!hasClkCls)) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error,
                "Unsupported message: stream class has no default clock class: "
                "msg-type={}",
                msg.type());
        }
    };

    using OptClkSnapshot = bt2::OptionalBorrowedObject<bt2::ConstClockSnapshot>;

    const auto clkSnapshot = std::invoke([&]() -> OptClkSnapshot {
        switch (msg.type()) {
        case bt2::MessageType::Event:
        {
            const auto eventMsg = msg.asEvent();

            validateHasClkCls(eventMsg.streamClassDefaultClockClass().hasObject());
            return eventMsg.defaultClockSnapshot();
        }

        case bt2::MessageType::PacketBeginning:
        {
            const auto packetBeginningMsg = msg.asPacketBeginning();

            if (!packetBeginningMsg.packet().stream().cls().packetsHaveBeginningClockSnapshot()) {
                return {};
            }

            return packetBeginningMsg.defaultClockSnapshot();
        }

        case bt2::MessageType::PacketEnd:
        {
            const auto packetEndMsg = msg.asPacketEnd();

            if (!packetEndMsg.packet().stream().cls().packetsHaveEndClockSnapshot()) {
                return {};
            }

            return packetEndMsg.defaultClockSnapshot();
        }

        case bt2::MessageType::StreamBeginning:
        {
            const auto streamBeginningMsg = msg.asStreamBeginning();

            validateHasClkCls(streamBeginningMsg.streamClassDefaultClockClass().hasObject());
            return streamBeginningMsg.defaultClockSnapshot();
        }

        case bt2::MessageType::StreamEnd:
        {
            const auto streamEndMsg = msg.asStreamEnd();

            validateHasClkCls(streamEndMsg.streamClassDefaultClockClass().hasObject());
            return streamEndMsg.defaultClockSnapshot();
        }

        case bt2::MessageType::DiscardedEvents:
        {
            const auto discEventsMsg = msg.asDiscardedEvents();

            if (!discEventsMsg.stream().cls().discardedEventsHaveDefaultClockSnapshots()) {
                return {};
            }

            return discEventsMsg.beginningDefaultClockSnapshot();
        }

        case bt2::MessageType::DiscardedPackets:
        {
            const auto discPacketsMsg = msg.asDiscardedPackets();

            if (!discPacketsMsg.stream().cls().discardedPacketsHaveDefaultClockSnapshots()) {
                return {};
            }

            return discPacketsMsg.beginningDefaultClockSnapshot();
        }

        case bt2::MessageType::MessageIteratorInactivity:
            return msg.asMessageIteratorInactivity().clockSnapshot();

        default:
            return {};
        }
    });

    if (!clkSnapshot) {
        return std::nullopt;
    }

    try {
        return clkSnapshot->nsFromOrigin();
    } catch (const bt2c::OverflowError&) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error,
            "Cannot convert clock snapshot to nanoseconds from origin: msg-type={}, "
            "clock-snapshot-value={}",
            msg.type(), clkSnapshot->value());
    }
}

std::uint64_t MsgIter::_endClkVal(const bt2::ConstClockClass clkCls) const
{
    const auto offset = clkCls.offsetFromOrigin();
    std::uint64_t val;

    if (bt_common_clock_value_from_ns_from_origin(offset.seconds(), offset.cycles(),
                                                  clkCls.frequency(), _mEnd.nsFromOrig(), &val)) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error,
            "Cannot convert the end time of the trimming time range to a clock value: "
            "end-ns-from-origin={}, clock-class-name={}",
            _mEnd.nsFromOrig(), clkCls.name() ? clkCls.name().data() : "(none)");
    }

    return val;
}

bool MsgIter::_handleMsg(const bt2::ConstMessage msg)
{
    /* Find the associated stream of the message */
    const auto stream = std::invoke([msg]() -> bt2::OptionalBorrowedObject<bt2::ConstStream> {
        switch (msg.type()) {
        case bt2::MessageType::Event:
            return msg.asEvent().event().stream();
        case bt2::MessageType::PacketBeginning:
            return msg.asPacketBeginning().packet().stream();
        case bt2::MessageType::PacketEnd:
            return msg.asPacketEnd().packet().stream();
        case bt2::MessageType::DiscardedEvents:
            return msg.asDiscardedEvents().stream();
        case bt2::MessageType::DiscardedPackets:
            return msg.asDiscardedPackets().stream();
        case bt2::MessageType::StreamBeginning:
            return msg.asStreamBeginning().stream();
        case bt2::MessageType::StreamEnd:
            return msg.asStreamEnd().stream();
        default:
            return {};
        }
    });

    /* Retrieve the time of the message */
    const auto nsFromOrig = this->_msgNsFromOrig(msg);

    if (G_LIKELY(stream)) {
        /* Message associated to a stream */
        return this->_handleMsgWithStream(msg, *stream, nsFromOrig);
    }

    /* Message not associated to a stream (message iterator inactivity) */
    BT_ASSERT_DBG(nsFromOrig);

    if (G_UNLIKELY(_mEnd.isBefore(*nsFromOrig))) {
        this->_endStreams();
        return true;
    }

    _mOutputMsgs.emplace_back(msg.shared());
    return false;
}

bool MsgIter::_handleMsgWithStream(const bt2::ConstMessage msg, const bt2::ConstStream stream,
                                   const std::optional<std::int64_t>& nsFromOrig)
{
    switch (msg.type()) {
    case bt2::MessageType::Event:
    {
        /*
         * An event message always has a clock snapshot if its stream
         * class has a default clock class, which is a requirement of
         * this component.
         */
        BT_ASSERT_DBG(nsFromOrig);

        auto& streamState = this->_streamState(stream);

        if (G_UNLIKELY(_mEnd.isBefore(*nsFromOrig))) {
            this->_endStreams();
            return true;
        }

        streamState.seenClkSnapshot = true;
        _mOutputMsgs.emplace_back(msg.shared());
        break;
    }

    case bt2::MessageType::PacketBeginning:
    {
        /*
         * A packet beginning message doesn't have a clock snapshot if
         * the packets of its stream class have no beginning default
         * clock snapshot, but this component currently requires them
         * (see _createStreamState()).
         */
        BT_ASSERT(nsFromOrig);

        auto& streamState = this->_streamState(stream);

        BT_ASSERT(!streamState.curPacket);

        if (G_UNLIKELY(_mEnd.isBefore(*nsFromOrig))) {
            this->_endStreams();
            return true;
        }

        streamState.curPacket = msg.asPacketBeginning().packet().shared();
        streamState.seenClkSnapshot = true;
        _mOutputMsgs.emplace_back(msg.shared());
        break;
    }

    case bt2::MessageType::PacketEnd:
    {
        /*
         * A packet end message doesn't have a clock snapshot if the
         * packets of its stream class have no end default clock
         * snapshot, but this component currently requires them
         * (see _createStreamState()).
         */
        BT_ASSERT(nsFromOrig);

        auto& streamState = this->_streamState(stream);

        BT_ASSERT(streamState.curPacket);

        if (G_UNLIKELY(_mEnd.isBefore(*nsFromOrig))) {
            this->_endStreams();
            return true;
        }

        streamState.curPacket.reset();
        streamState.seenClkSnapshot = true;
        _mOutputMsgs.emplace_back(msg.shared());
        break;
    }

    case bt2::MessageType::DiscardedEvents:
    case bt2::MessageType::DiscardedPackets:
    {
        /*
         * `nsFromOrig` is the beginning time of the time range of
         * the message here.
         */
        BT_ASSERT(nsFromOrig);

        auto& streamState = this->_streamState(stream);

        streamState.seenClkSnapshot = true;

        const auto isDiscEvents = msg.isDiscardedEvents();
        const auto endClkSnapshot = isDiscEvents ?
                                        msg.asDiscardedEvents().endDefaultClockSnapshot() :
                                        msg.asDiscardedPackets().endDefaultClockSnapshot();
        const auto endNsFromOrig = std::invoke([this, msg, endClkSnapshot] {
            try {
                return endClkSnapshot.nsFromOrigin();
            } catch (const bt2c::OverflowError&) {
                BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                    bt2c::Error,
                    "Cannot convert end clock snapshot to nanoseconds from origin: "
                    "msg-type={}, clock-snapshot-value={}",
                    msg.type(), endClkSnapshot.value());
            }
        });

        if (_mEnd.isBefore(*nsFromOrig)) {
            this->_endStreams();
            return true;
        }

        if (_mEnd.isBefore(endNsFromOrig)) {
            /*
             * The end time of this message is outside the trimming
             * time range: replace it with a new message having an end
             * time equal to the end time of the trimming time range
             * and without a count.
             */
            const auto endClkVal = this->_endClkVal(endClkSnapshot.clockClass());

            if (isDiscEvents) {
                _mOutputMsgs.emplace_back(this->_createDiscardedEventsMessage(
                    *streamState.stream,
                    msg.asDiscardedEvents().beginningDefaultClockSnapshot().value(), endClkVal));
            } else {
                _mOutputMsgs.emplace_back(this->_createDiscardedPacketsMessage(
                    *streamState.stream,
                    msg.asDiscardedPackets().beginningDefaultClockSnapshot().value(), endClkVal));
            }
        } else {
            _mOutputMsgs.emplace_back(msg.shared());
        }

        break;
    }

    case bt2::MessageType::StreamBeginning:
    {
        /*
         * If this message has a time and this time is greater than the
         * end time of the trimming time range, then it triggers the
         * end of the trimming time range.
         */
        if (G_UNLIKELY(nsFromOrig && _mEnd.isBefore(*nsFromOrig))) {
            this->_endStreams();
            return true;
        }

        /* Learn about this stream */
        auto& streamState = this->_createStreamState(stream);

        if (nsFromOrig) {
            streamState.seenClkSnapshot = true;
        }

        _mOutputMsgs.emplace_back(msg.shared());
        break;
    }

    case bt2::MessageType::StreamEnd:
    {
        /*
         * If this message has a time and this time is greater than the
         * end time of the trimming time range, then it triggers the
         * end of the trimming time range.
         */
        if (G_UNLIKELY(nsFromOrig && _mEnd.isBefore(*nsFromOrig))) {
            this->_endStreams();
            return true;
        }

        /*
         * Either the time of the stream end message is within the
         * trimming time range, or it doesn't have a time: in both
         * cases, pass the message unmodified.
         */
        _mOutputMsgs.emplace_back(msg.shared());

        /* Forget about this stream */
        const auto erasedCount = _mStreamStates.erase(stream.libObjPtr());

        BT_ASSERT(erasedCount == 1);
        break;
    }

    default:
        break;
    }

    return false;
}

MsgIter::_StreamState& MsgIter::_createStreamState(const bt2::ConstStream stream)
{
    BT_ASSERT(_mStreamStates.find(stream.libObjPtr()) == _mStreamStates.end());

    /* Returns the logging details of `stream` */
    const auto streamDetails = [stream] {
        const auto name = stream.name();

        return fmt::format("stream-addr={}, stream-id={}, stream-name={}",
                           fmt::ptr(stream.libObjPtr()), stream.id(),
                           name ? fmt::format("\"{}\"", name) : "(none)");
    };

    /*
     * Validate right now that the class of the stream has a default
     * clock class so that an existing stream state guarantees existing
     * default clock snapshots for its associated messages.
     *
     * Also check that clock snapshots are always known.
     */
    const auto streamCls = stream.cls();

    if (!streamCls.defaultClockClass()) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error, "Unsupported stream: stream class does not have a default clock class: {}",
            streamDetails());
    }

    /*
     * Temporary: make sure packet beginning, packet end, discarded
     * events, and discarded packets messages have default clock
     * snapshots until the support for not having them is implemented.
     */
    if (streamCls.supportsPackets()) {
        if (!streamCls.packetsHaveBeginningClockSnapshot()) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "Unsupported stream: packets have no beginning clock snapshot: {}",
                streamDetails());
        }

        if (!streamCls.packetsHaveEndClockSnapshot()) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "Unsupported stream: packets have no end clock snapshot: {}",
                streamDetails());
        }

        if (streamCls.supportsDiscardedPackets() &&
            !streamCls.discardedPacketsHaveDefaultClockSnapshots()) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "Unsupported stream: discarded packets have no clock snapshots: {}",
                streamDetails());
        }
    }

    if (streamCls.supportsDiscardedEvents() &&
        !streamCls.discardedEventsHaveDefaultClockSnapshots()) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error, "Unsupported stream: discarded events have no clock snapshots: {}",
            streamDetails());
    }

    return _mStreamStates.emplace(stream.libObjPtr(), _StreamState {stream}).first->second;
}

MsgIter::_StreamState& MsgIter::_streamState(const bt2::ConstStream stream) noexcept
{
    const auto it = _mStreamStates.find(stream.libObjPtr());

    BT_ASSERT_DBG(it != _mStreamStates.end());
    return it->second;
}

void MsgIter::_endStreams()
{
    if (!_mEnd.isInfinite()) {
        /*
         * End each stream and then remove them from the stream states
         * to release unneeded references.
         *
         * An infinite end time of the trimming time range guarantees
         * that we received (and pushed) all the appropriate
         * end messages.
         */
        for (auto& streamStatePair : _mStreamStates) {
            this->_endStream(streamStatePair.second);
        }
    }

    _mStreamStates.clear();
}

void MsgIter::_endStream(_StreamState& streamState)
{
    BT_ASSERT(!_mEnd.isInfinite());
    BT_ASSERT(streamState.stream);

    /*
     * If we haven't seen a message with a clock snapshot, then we don't
     * know if the end time of the trimming time range is within the
     * range of the clock, so it wouldn't be safe to try to convert it
     * to a clock value.
     *
     * Also, it would be a bit of a lie to generate a stream end message
     * with the end time of the trimming time range as its clock
     * snapshot, because we don't really know if the stream existed at
     * that time. If we have seen a message with a clock snapshot and
     * another message with a clock snapshot cuts the stream short, then
     * we're sure that the end time of the trimming time range is not
     * below the clock range, and we know the stream was active at that
     * time (and that we cut it short).
     */
    std::uint64_t endClkVal = 0;

    if (streamState.seenClkSnapshot) {
        endClkVal = this->_endClkVal(*streamState.stream->cls().defaultClockClass());
    }

    if (streamState.curPacket) {
        /*
         * Create and push a packet end message, making its time the end
         * time of the trimming time range.
         *
         * We know that we must have seen a clock snapshot, the one of
         * the packet beginning message, since this component currently
         * requires packet messages to have clock snapshots
         * (see _createStreamState()).
         */
        BT_ASSERT(streamState.seenClkSnapshot);
        _mOutputMsgs.emplace_back(this->_createPacketEndMessage(*streamState.curPacket, endClkVal));
        streamState.curPacket.reset();
    }

    /* Create and push a stream end message */
    const auto streamEndMsg = this->_createStreamEndMessage(*streamState.stream);

    if (streamState.seenClkSnapshot) {
        streamEndMsg->defaultClockSnapshot(endClkVal);
    }

    _mOutputMsgs.emplace_back(streamEndMsg);

    /*
     * Just to make sure that we don't use this stream state again in
     * the future without an obvious error.
     */
    streamState.stream.reset();
}

} /* namespace bt2trimmer */
