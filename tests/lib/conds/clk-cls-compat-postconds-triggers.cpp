/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2024 EfficiOS Inc.
 */

#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

#include <fmt/format.h> /* IWYU pragma: keep */

#include "cpp-common/bt2c/uuid.hpp"

#include "common.hpp"
#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

enum class MsgType
{
    StreamBeg,
    MsgIterInactivity,
};

/*
 * Configures a freshly created clock class for a single message of the
 * two-message stream that clock class compatibility triggers build.
 *
 * An empty `std::function` means "no clock class": the helper builds
 * the message without one, which itself drives some triggers (for
 * example, `*:stream-class-has-no-clock-class`).
 *
 * A non-empty function means "create a default clock class and run this
 * configurator on it". Use `defaultClkClsFunc` for the common "default
 * clock class, no further tweaks" case.
 */
using ClkClsCfgFunc = std::function<void(bt2::ClockClass)>;

const ClkClsCfgFunc defaultClkClsFunc = [](auto) {
};

__attribute__((used)) const char *format_as(const MsgType msgType)
{
    switch (msgType) {
    case MsgType::StreamBeg:
        return "sb";

    case MsgType::MsgIterInactivity:
        return "mii";
    }

    bt_common_abort();
}

bt2::Message::Shared createOneMsg(const bt2::SelfMessageIterator selfMsgIter, const MsgType msgType,
                                  const ClkClsCfgFunc& clkClsCfgFunc, const bt2::Trace trace)
{
    bt2::ClockClass::Shared clkCls;

    if (clkClsCfgFunc) {
        clkCls = selfMsgIter.component().createClockClass();
        clkClsCfgFunc(*clkCls);
    }

    switch (msgType) {
    case MsgType::StreamBeg:
    {
        const auto streamCls = trace.cls().createStreamClass();

        if (clkCls) {
            streamCls->defaultClockClass(*clkCls);
        }

        return selfMsgIter.createStreamBeginningMessage(*streamCls->instantiate(trace));
    }

    case MsgType::MsgIterInactivity:
        BT_ASSERT(clkCls);
        return selfMsgIter.createMessageIteratorInactivityMessage(*clkCls, 12);
    };

    bt_common_abort();
}

void addClkClsCompatTrigger(CondTriggers& triggers, const MsgType msgType1,
                            ClkClsCfgFunc clkClsCfgFunc1, const MsgType msgType2,
                            ClkClsCfgFunc clkClsCfgFunc2, const char * const condId,
                            const std::uint64_t graphMipVersion, const std::string_view nameSuffix)
{
    triggers.emplace_back(makeRunInMsgIterNextTrigger(
        [msgType1, msgType2, clkClsCfg1 = std::move(clkClsCfgFunc1),
         clkClsCfg2 = std::move(clkClsCfgFunc2)](const auto selfMsgIter, auto& msgs) {
            const auto trace = selfMsgIter.component().createTraceClass()->instantiate();

            msgs.append(createOneMsg(selfMsgIter, msgType1, clkClsCfg1, *trace));
            msgs.append(createOneMsg(selfMsgIter, msgType2, clkClsCfg2, *trace));
        },
        CondTrigger::Type::Post, condId, graphMipVersion, nameSuffix));
}

const bt2c::Uuid uuidA {"f00aaf65-ebec-4eeb-85b2-fc255cf1aa8a"};
const bt2c::Uuid uuidB {"03482981-a77b-4d7b-94c4-592bf9e91785"};
constexpr const char *nsA = "namespace-a";
constexpr const char *nameA = "name-a";
constexpr const char *uidA = "uid-a";
constexpr const char *nsB = "namespace-b";
constexpr const char *nameB = "name-b";
constexpr const char *uidB = "uid-b";

} /* namespace */

/*
 * Adds clock class compatibility postcondition failure triggers.
 */
void addClkClsCompatTriggers(CondTriggers& triggers)
{
    const auto addValidCases = [&triggers](const auto& clkClsCfgFunc1, const auto& clkClsCfgFunc2,
                                           const char * const condId, const auto graphMipVersion) {
        /*
         * Add triggers for all possible combinations of message types.
         *
         * It's not possible to create message iterator inactivity messages
         * without a clock class.
         */
        static constexpr std::array msgTypes {
            MsgType::StreamBeg,
            MsgType::MsgIterInactivity,
        };

        const auto isInvalidCase = [](const auto msgType, const auto& clkClsCfgFunc) {
            return msgType == MsgType::MsgIterInactivity && !ClkClsCfgFunc {clkClsCfgFunc};
        };

        for (const auto msgType1 : msgTypes) {
            if (isInvalidCase(msgType1, clkClsCfgFunc1)) {
                continue;
            }

            for (const auto msgType2 : msgTypes) {
                if (isInvalidCase(msgType2, clkClsCfgFunc2)) {
                    continue;
                }

                addClkClsCompatTrigger(
                    triggers, msgType1, clkClsCfgFunc1, msgType2, clkClsCfgFunc2, condId,
                    graphMipVersion,
                    fmt::format("mip{}-{}-{}", graphMipVersion, msgType1, msgType2));
            }
        }
    };

    forEachMipVersion([&](const auto graphMipVersion) {
        addValidCases(ClkClsCfgFunc {}, defaultClkClsFunc,
                      "message-iterator-class-next-method:stream-class-has-no-clock-class",
                      graphMipVersion);

        if (graphMipVersion == 0) {
            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                ClkClsCfgFunc {},
                "message-iterator-class-next-method:stream-class-has-clock-class-with-unix-epoch-origin",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false);
                },
                "message-iterator-class-next-method:clock-class-has-unix-epoch-origin",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).uuid(uuidA);
                },
                ClkClsCfgFunc {},
                "message-iterator-class-next-method:stream-class-has-clock-class-with-uuid",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).uuid(uuidA);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                "message-iterator-class-next-method:clock-class-has-unknown-origin",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).uuid(uuidA);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false);
                },
                "message-iterator-class-next-method:clock-class-has-uuid", graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).uuid(uuidA);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).uuid(uuidB);
                },
                "message-iterator-class-next-method:clock-class-has-expected-uuid",
                graphMipVersion);
        } else {
            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                ClkClsCfgFunc {},
                "message-iterator-class-next-method:stream-class-has-clock-class-with-known-origin",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false);
                },
                "message-iterator-class-next-method:clock-class-has-known-origin", graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).nameSpace("ze-ns").name("ze-name").uid(
                        "ze-uid");
                },
                ClkClsCfgFunc {},
                "message-iterator-class-next-method:stream-class-has-clock-class-with-id",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).nameSpace("ze-ns").name("ze-name").uid(
                        "ze-uid");
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(true);
                },
                "message-iterator-class-next-method:clock-class-has-unknown-origin",
                graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).nameSpace(nsA).name(nameA).uid(uidA);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false);
                },
                "message-iterator-class-next-method:clock-class-has-id", graphMipVersion);

            addValidCases(
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).nameSpace(nsA).name(nameA).uid(uidA);
                },
                [](auto clkCls) {
                    clkCls.originIsUnixEpoch(false).nameSpace(nsB).name(nameB).uid(uidB);
                },
                "message-iterator-class-next-method:clock-class-has-expected-id", graphMipVersion);
        }

        addValidCases(
            [](auto clkCls) {
                clkCls.originIsUnixEpoch(false);
            },
            ClkClsCfgFunc {}, "message-iterator-class-next-method:stream-class-has-clock-class",
            graphMipVersion);

        addValidCases(
            [](auto clkCls) {
                clkCls.originIsUnixEpoch(false);
            },
            [](auto clkCls) {
                clkCls.originIsUnixEpoch(false);
            },
            "message-iterator-class-next-method:clock-class-is-expected", graphMipVersion);
    });
}
