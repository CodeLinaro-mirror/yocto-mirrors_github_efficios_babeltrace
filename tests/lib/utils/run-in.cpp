/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2020-2023 EfficiOS, inc.
 */

#include "common/assert.h"
#include "cpp-common/bt2/component-class-dev.hpp"
#include "cpp-common/bt2/component-class.hpp"
#include "cpp-common/bt2/graph.hpp"
#include "cpp-common/bt2/message-iterator.hpp"
#include "cpp-common/bt2/query-executor.hpp"

#include "run-in.hpp"

void RunIn::onQuery(bt2::SelfComponentClass)
{
}

void RunIn::onCompInit(bt2::SelfComponent)
{
}

void RunIn::onMsgIterInit(bt2::SelfMessageIterator)
{
}

void RunIn::onMsgIterNext(bt2::SelfMessageIterator, bt2::ConstMessageArray&)
{
}

namespace {

class RunInSource;

class RunInSourceMsgIter final : public bt2::UserMessageIterator<RunInSourceMsgIter, RunInSource>
{
public:
    explicit RunInSourceMsgIter(const bt2::SelfMessageIterator self,
                                bt2::SelfMessageIteratorConfiguration,
                                const bt2::SelfComponentOutputPort port)
        : bt2::UserMessageIterator<RunInSourceMsgIter, RunInSource> {self, "RUN-IN-SRC-MSG-ITER"},
          _mRunIn {&port.data<RunIn>()},
          _mSelf {self}
    {
        _mRunIn->onMsgIterInit(self);
    }

    void _next(bt2::ConstMessageArray& msgs)
    {
        _mRunIn->onMsgIterNext(_mSelf, msgs);
    }

private:
    RunIn *_mRunIn;
    bt2::SelfMessageIterator _mSelf;
};

class RunInSource final :
    public bt2::UserSourceComponent<RunInSource, RunInSourceMsgIter, RunIn, RunIn>
{
public:
    static constexpr auto name = "run-in-src";

    explicit RunInSource(const bt2::SelfSourceComponent self, bt2::ConstMapValue,
                         RunIn * const runIn)
        : bt2::UserSourceComponent<RunInSource, RunInSourceMsgIter, RunIn, RunIn> {self,
                                                                                   "RUN-IN-SRC"},
          _mRunIn {runIn}
    {
        this->_addOutputPort("out", *runIn);
        _mRunIn->onCompInit(self);
    }

    static bt2::Value::Shared _query(const bt2::SelfComponentClass self, bt2::PrivateQueryExecutor,
                                     bt2c::CStringView, bt2::ConstValue, RunIn *data)
    {
        data->onQuery(self);
        return bt2::NullValue {}.shared();
    }

private:
    RunIn *_mRunIn;
};

/*
 * Inline version of `sink.utils.dummy` to avoid loading plugins on
 * every test.
 */
class RunInSink final : public bt2::UserSinkComponent<RunInSink>
{
public:
    static constexpr auto name = "run-in-sink";

    explicit RunInSink(const bt2::SelfSinkComponent self, bt2::ConstMapValue, void *)
        : bt2::UserSinkComponent<RunInSink> {self, "RUN-IN-SINK"}
    {
        this->_addInputPort("in");
    }

    void _graphIsConfigured()
    {
        _mMsgIter = this->_createMessageIterator(this->_inputPorts()["in"]);
    }

    bool _consume()
    {
        return _mMsgIter->next().has_value();
    }

private:
    bt2::MessageIterator::Shared _mMsgIter;
};

} /* namespace */

void runIn(RunIn& runIn, const std::uint64_t graphMipVersion)
{
    const auto srcCompCls = bt2::createComponentClass<RunInSource>();

    /* Execute a query */
    bt2::QueryExecutor::create(*srcCompCls, "object-name", runIn)->query();

    /* Create graph */
    const auto graph = bt2::Graph::create(graphMipVersion);

    /* Add custom source component (executes `compCtxFunc`) */
    const auto srcComp = graph->addComponent(*srcCompCls, "the-source", runIn);

    /* Add custom sink component */
    const auto sinkCompCls = bt2::createComponentClass<RunInSink>();
    const auto sinkComp = graph->addComponent(*sinkCompCls, "the-sink");

    /* Connect ports */
    const auto outPort = srcComp.outputPorts()["out"];
    BT_ASSERT(outPort);

    const auto inPort = sinkComp.inputPorts()["in"];
    BT_ASSERT(inPort);

    graph->connectPorts(*outPort, *inPort);

    /* Run graph (executes `msgIterCtxFunc`) */
    graph->run();
}
