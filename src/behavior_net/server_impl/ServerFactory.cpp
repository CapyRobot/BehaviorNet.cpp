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

#include <behavior_net/Common.hpp>
#include <behavior_net/Controller.hpp>
#include <behavior_net/Types.hpp>
#include <behavior_net/server_impl/HttpServer.hpp>

namespace capybot
{
namespace bnet
{

std::unique_ptr<IServer> IServer::create(nlohmann::json const& controllerConfig,
                                         ControllerCallbacks const& controllerCbs)
{
    if (controllerConfig.contains("http_server"))
    {
        return std::make_unique<HttpServer>(controllerConfig.at("http_server"), controllerCbs);
    }

    std::cerr << "[IServer::create] no server in config file - running serverless." << std::endl;
    return nullptr;
}

} // namespace bnet
} // namespace capybot