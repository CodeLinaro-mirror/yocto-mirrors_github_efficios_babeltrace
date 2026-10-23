/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2019-2026 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_UTILS_TRIMMER_MSG_ITER_HPP
#define BABELTRACE_PLUGINS_UTILS_TRIMMER_MSG_ITER_HPP

#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/component-class-dev.hpp"
#include "cpp-common/bt2/message-iterator.hpp"
#include "cpp-common/bt2/message.hpp"
#include "cpp-common/bt2/self-message-iterator-configuration.hpp"
#include "cpp-common/bt2/trace-ir.hpp"

#include "bound.hpp"

namespace bt2trimmer {

class Comp;

class MsgIter final : public bt2::UserMessageIterator<MsgIter, Comp>
{
    friend bt2::UserMessageIterator<MsgIter, Comp>;

private:
    enum class _State
    {
        /*
         * Find the time of the first message having one and set the
         * date of the time of day bounds accordingly.
         */
        SetBoundsNsFromOrig,

        /*
         * Initially seek to the beginning time of the trimming
         * time range.
         */
        SeekInitially,

        /*
         * Fill the output message queue for as long as received input
         * messages are within the trimming time range.
         */
        Trim,

        /* Flush the remaining messages of the output message queue */
        Ending,

        /* Trimming operation and message iterator are ended */
        Ended,
    };

    /*
     * State of a stream.
     */
    struct _StreamState final
    {
        explicit _StreamState(const bt2::ConstStream streamParam)
            : stream {streamParam.shared()}
        {
        }

        bt2::ConstStream::Shared stream;

        /*
         * Whether or not we've seen a message having a clock snapshot
         * for this stream.
         */
        bool seenClkSnapshot = false;

        /* Current packet (empty initially and between packets) */
        bt2::ConstPacket::Shared curPacket;
    };

public:
    explicit MsgIter(bt2::SelfMessageIterator selfMsgIter,
                     bt2::SelfMessageIteratorConfiguration config,
                     bt2::SelfComponentOutputPort selfPort);

private:
    void _next(bt2::ConstMessageArray& msgs);

    /*
     * Finds the time of the first upstream message having one and sets
     * the date of the time of day bounds accordingly, discarding all
     * the received upstream messages.
     *
     * Returns `false` if the upstream message iterator ended before
     * this method could find a message having a time.
     */
    bool _setBoundsNsFromOrig();

    /*
     * Makes the upstream message iterator seek the beginning time of
     * the trimming time range.
     */
    void _seekInitially();

    /*
     * Fills the output message queue with trimmed messages from the
     * upstream message iterator until it's not empty, and then moves
     * messages to `msgs`.
     */
    void _trim(bt2::ConstMessageArray& msgs);

    /*
     * Moves messages from the output message queue to `msgs`, setting
     * the state to `_State::Ended` if there's none.
     */
    void _ending(bt2::ConstMessageArray& msgs);

    /*
     * Moves as many messages as possible from the output message queue
     * (not empty) to `msgs`.
     */
    void _moveOutputMsgs(bt2::ConstMessageArray& msgs);

    /*
     * Returns the time (nanoseconds from origin) of `msg`, or
     * `std::nullopt` if it doesn't have any.
     */
    std::optional<std::int64_t> _msgNsFromOrig(bt2::ConstMessage msg) const;

    /*
     * Returns the value of the clock snapshot of a clock of class
     * `clkCls` corresponding to the end time of the trimming
     * time range.
     */
    std::uint64_t _endClkVal(bt2::ConstClockClass clkCls) const;

    /*
     * Handles the upstream message `msg`.
     *
     * This _could_ make the output message queue grow; this could also
     * consume the message without pushing anything to this queue, only
     * modifying a stream state.
     *
     * Returns `true` if handling this message made this message
     * iterator reach the end of the trimming time range. Note that the
     * output message queue could contain messages even in that case.
     */
    bool _handleMsg(bt2::ConstMessage msg);

    /*
     * Like _handleMsg(), but for a message `msg` associated to the
     * stream `stream`.
     *
     * `nsFromOrig` is the time of `msg`, as _msgNsFromOrig()
     * returns it.
     */
    bool _handleMsgWithStream(bt2::ConstMessage msg, bt2::ConstStream stream,
                              const std::optional<std::int64_t>& nsFromOrig);

    /*
     * Creates and returns a state for the new stream `stream`.
     */
    _StreamState& _createStreamState(bt2::ConstStream stream);

    /*
     * Returns the existing state of the stream `stream`.
     */
    _StreamState& _streamState(bt2::ConstStream stream) noexcept;

    /*
     * Ends all the streams of which this message iterator has a state
     * (if the end time of the trimming time range isn't infinite) and
     * removes all the stream states.
     */
    void _endStreams();

    /*
     * Pushes, to the output message queue, the messages which end the
     * stream of the state `streamState`.
     */
    void _endStream(_StreamState& streamState);

    _State _mState;
    bt2::MessageIterator::Shared _mUpstreamMsgIter;

    /* Copies of the bounds of the component, possibly updated */
    Bound _mBegin;
    Bound _mEnd;

    /*
     * Queue of messages which this message iterator is about to
     * output, oldest first.
     */
    std::deque<bt2::ConstMessage::Shared> _mOutputMsgs;

    /* Stream states */
    std::unordered_map<const bt_stream *, _StreamState> _mStreamStates;
};

} /* namespace bt2trimmer */

#endif /* BABELTRACE_PLUGINS_UTILS_TRIMMER_MSG_ITER_HPP */
