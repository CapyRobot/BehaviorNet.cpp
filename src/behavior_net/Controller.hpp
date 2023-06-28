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

#pragma once

#include <3rd_party/better_enums/enums.h>
#include <behavior_net/Action.hpp>
#include <behavior_net/PetriNet.hpp>

#include <3rd_party/cpp-httplib/httplib.h>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>

namespace capybot
{
namespace bnet
{

struct ControllerCallbacks
{
    std::function<void(nlohmann::json const& contentBlocks, std::string_view placeId)> addToken;
    std::function<nlohmann::json()> getNetMarking;
    std::function<void(std::string_view const& id)> triggerManualTransition;
};

BETTER_ENUM(ServerType, uint32_t, HTTP);

class IServer
{
    static constexpr const char* MODULE_TAG{"IServer"};

public:
    static std::unique_ptr<IServer> create(nlohmann::json const& controllerConfig,
                                           ControllerCallbacks const& controllerCbs);

    virtual void start() = 0;
    virtual void stop() = 0;
};

class Controller
{
    static constexpr const char* MODULE_TAG{"Controller"};

public:
    Controller(NetConfig const& config, std::unique_ptr<PetriNet> petriNet);

    ~Controller() { stop(); }

    void addToken(nlohmann::json const& contentBlocks, std::string_view placeId);

    void run();

    void runDetached();

    void stop();

    void runEpoch();

    PetriNet const& getNet() const { return *m_net; }
    PetriNet& getNet() { return *m_net; }

private:
    ControllerCallbacks createCallbacks();

    ThreadPool m_tp;
    nlohmann::json const& m_config;

    std::atomic_bool m_running{false};
    std::thread m_runDetachedThread;

    std::unique_ptr<PetriNet> m_net;
    std::unique_ptr<IServer> m_server;
};

} // namespace bnet
} // namespace capybot
