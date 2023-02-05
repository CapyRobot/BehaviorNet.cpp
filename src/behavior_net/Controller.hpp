/*
 * Copyright (C) 2023 Eduardo Rocha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BEHAVIOR_NET_CPP_CONTROLLER_HPP_
#define BEHAVIOR_NET_CPP_CONTROLLER_HPP_

#include "behavior_net/Action.hpp"
#include "behavior_net/PetriNet.hpp"

#include <iostream>

namespace capybot
{
namespace bnet
{

class Controller
{
public:
    Controller(NetConfig const& config, std::unique_ptr<PetriNet> petriNet)
        : m_tp(config.get().at("controller").at("thread_poll_workers").get<uint32_t>()),
          m_config(config.get().at("controller")), m_net(std::move(petriNet))
    {
        Place::createActions(m_tp, config.get().at("controller").at("actions"), m_net->getPlaces());
    }

    ~Controller() { stop(); }

    void addToken(nlohmann::json const& contentBlocks, std::string_view placeId)
    {
        Token token;
        for (auto&& block : contentBlocks)
        {
            token.addContentBlock(block.at("key"), block.at("content"));
        }
        m_net->addToken(token, placeId);

        m_net->prettyPrintState();
    }

    void run()
    {
        m_running.store(true);
        while (m_running.load())
        {
            runEpoch();
        }
    }

    void runDetached()
    {
        // TODO:assert joinable
        m_runDetachedThread = std::thread([this] { run(); });
    }

    void stop()
    {
        m_running.store(false);
        if (m_runDetachedThread.joinable())
        {
            m_runDetachedThread.join();
        }
    }

    void runEpoch()
    {
        log::timePoint("runEpoch start...");
        const uint32_t periodMs = m_config.at("epoch_period_ms").get<uint32_t>();
        // const uint32_t sleepMs = std::max(1U, periodMs / 10U);

        // execute all actions
        for (auto&& [_, place] : m_net->getPlaces())
        {
            place->executeActionAsync();
        }

        // wait
        std::this_thread::sleep_for(std::chrono::milliseconds(periodMs)); // TODO: sleep until

        // wait for tasks to complete
        for (auto&& [_, place] : m_net->getPlaces())
        {
            place->checkActionResults();
        }

        // fire all enabled transitions
        for (auto&& t : m_net->getTransitions())
        {
            while (t.isEnabled()) // TODO
            {
                t.trigger();
            }
        }

        log::timePoint("runEpoch ... done");
        m_net->prettyPrintState();
    }

private:
    ThreadPool m_tp;
    nlohmann::json const& m_config;

    std::atomic<bool> m_running{false}; // TODO: error handling on logic
    std::thread m_runDetachedThread;

    std::unique_ptr<PetriNet> m_net;
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_CONTROLLER_HPP_