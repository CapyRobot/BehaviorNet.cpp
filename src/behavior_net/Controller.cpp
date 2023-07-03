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

#include <behavior_net/Controller.hpp>
#include <behavior_net/Types.hpp>
#include <utils/Logger.hpp>

namespace capybot
{
namespace bnet
{

Controller::Controller(NetConfig const& config, std::unique_ptr<PetriNet> petriNet)
    : m_tp(config.get().at("controller").at("thread_poll_workers").get<uint32_t>())
    , m_config(config.get().at("controller"))
    , m_net(std::move(petriNet))
    , m_server(IServer::create(config.get().at("controller"), createCallbacks()))
{
    Place::Factory::createActions(m_tp, config.get().at("controller").at("actions"), m_net->getPlaces());
}

void Controller::addToken(nlohmann::json const& contentBlocks, std::string_view placeId)
{
    LOG(DEBUG) << "addToken @ " << placeId << "; content = " << contentBlocks << log::endl;

    auto token = Token::makeUnique();
    for (nlohmann::json::const_iterator it = contentBlocks.begin(); it != contentBlocks.end(); ++it)
    {
        token->addContentBlock(it.key(), it.value());
    }
    m_net->addToken(token, placeId);

    m_net->prettyPrintState();
}

void Controller::run()
{
    SCOPED_LOG_TRACER("run");

    if (m_running.load())
    {
        throw Exception(ExceptionType::LOGIC_ERROR, "[Controller::run] controller is already running.");
    }

    LOG(INFO) << "run: running... " << log::endl;

    m_running.store(true);
    if (m_server)
    {
        m_server->start();
    }
    while (m_running.load())
    {
        // TODO: cli arg to activate m_net->prettyPrintState();
        runEpoch();
    }
}

void Controller::runDetached()
{
    if (m_running.load() || m_runDetachedThread.joinable())
    {
        throw Exception(ExceptionType::LOGIC_ERROR, "[Controller::runDetached] controller is already running.");
    }
    m_runDetachedThread = std::thread([this] { run(); });
}

void Controller::stop()
{
    SCOPED_LOG_TRACER("stop");

    m_running.store(false);
    if (m_server)
    {
        m_server->stop();
    }
    if (m_runDetachedThread.joinable())
    {
        m_runDetachedThread.join();
    }
}

void Controller::runEpoch()
{
    SCOPED_LOG_TRACER("runEpoch");

    const uint32_t periodMs = m_config.at("epoch_period_ms").get<uint32_t>();

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

    // fire all enabled auto transitions
    for (auto&& t : m_net->getTransitions())
    {
        if (t.isManual())
            continue;

        // current logic is to trigger a transition only once per epoch
        // TODO: this should be configurable as this logic does not fulfill all use cases
        if (t.isEnabled())
        {
            t.trigger();
        }
    }
}

ControllerCallbacks Controller::createCallbacks()
{
    return ControllerCallbacks{
        .addToken = [this](nlohmann::json const& contentBlocks,
                           std::string_view placeId) { addToken(contentBlocks, placeId); },
        .getNetMarking = [this]() -> nlohmann::json { return getNet().getMarking(); },
        .triggerManualTransition = [this](std::string_view const& id) { getNet().triggerTransition(id, true); }};
}

} // namespace bnet
} // namespace capybot
