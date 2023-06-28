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

#include <behavior_net/Controller.hpp>
#include <memory>

namespace capybot
{
namespace bnet
{

class HttpServer : public IServer
{
    static constexpr const char* MODULE_TAG{"HttpServer"};

public:
    HttpServer(nlohmann::json const& config, ControllerCallbacks const& controllerCbs);

    ~HttpServer() { stop(); }

    void start() override
    {
        m_executionThread = std::thread([this] { runServer(); });
    }

    void stop() override
    {
        m_server->stop();
        if (m_executionThread.joinable())
        {
            m_executionThread.join();
        }
    }

private:
    void runServer();

    void setCallbacks(httplib::Server& server);

    httplib::Server* m_server{nullptr};
    ControllerCallbacks m_controllerCbs;

    std::string m_addr;
    int m_port;

    std::thread m_executionThread;
};

} // namespace bnet
} // namespace capybot
