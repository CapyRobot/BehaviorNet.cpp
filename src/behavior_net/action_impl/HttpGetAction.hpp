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

#include "behavior_net/Action.hpp"
#include "behavior_net/ActionRegistry.hpp"
#include "behavior_net/ConfigParameter.hpp"

#include "3rd_party/cpp-httplib/httplib.h"

#include <list>

namespace capybot
{
namespace bnet
{

/**
 * @brief This action uses HTTP GET requests for executing actions in a client entity
 *
 * The action first sends a request to trigger execution once. Until the client side action is done, the action sends
 * requests to query the action status. These two types of requests can have different paths, e.g.,
 * <host>:<port>/execute/action/abc and <host>:<port>/status/action/abc.
 *
 * Config parameters:
 *     "host"            [string] request host address
 *     "port"            [int] request port
 *     "execute_path"    [string] request path for starting execution, e.g., <host>:<port></execute/path>
 *     "get_status_path" [string] request path for getting execution status, e.g., <host>:<port></status/path>
 */
class HttpGetAction : public IActionImpl
{
public:
    HttpGetAction(nlohmann::json const config)
        : m_host(config.at("host"))
        , m_port(config.at("port"))
        , m_executePath(config.at("execute_path"))
        , m_getStatusPath(config.at("get_status_path"))
    {
    }

    std::function<ActionExecutionStatus()> createCallable(Token::ConstSharedPtr token) override
    {
        return [&token, this]() -> ActionExecutionStatus {
            auto host = m_host.get(token);
            auto port = m_port.get(token);
            auto executePath = m_executePath.get(token);
            auto getStatusPath = m_getStatusPath.get(token);

            auto const actionId = host + std::to_string(port) + executePath;
            bool const isInExecution = std::find(m_inExec.begin(), m_inExec.end(), actionId) != m_inExec.end();

            ActionExecutionStatus retStatus{ActionExecutionStatus::NOT_STARTED};
            if (isInExecution)
            {
                retStatus = request(host, port, getStatusPath);
                if (retStatus._value != ActionExecutionStatus::IN_PROGRESS)
                {
                    m_inExec.remove(actionId);
                }
            }
            else
            {
                retStatus = request(host, port, executePath);
                if (retStatus == +ActionExecutionStatus::IN_PROGRESS)
                {
                    m_inExec.push_back(actionId);
                }
            }
            return retStatus;
        };
    }

private:
    ActionExecutionStatus request(std::string const& host, int port, std::string const& path)
    {
        httplib::Client client(host, port);
        httplib::Result res = client.Get(path);

        std::stringstream logMsg;
        logMsg << "HttpGetAction :: requesting @ " << host << ":" << port << path << " ... ";

        ActionExecutionStatus retStatus(ActionExecutionStatus::NOT_STARTED);
        if (!res)
        {
            auto err = res.error();
            logMsg << "ERROR; HTTP error: " << httplib::to_string(err) << "\n";
            retStatus = ActionExecutionStatus::ERROR;
        }
        else if (res->status < 200 && res->status >= 300)
        {
            logMsg << "ERROR; response status code: " << res->status << "\n";
            retStatus = ActionExecutionStatus::ERROR;
        }
        else
        {
            better_enums::optional<ActionExecutionStatus> maybeRetStatus =
                ActionExecutionStatus::_from_string_nothrow(res->body.c_str());

            if (maybeRetStatus)
            {
                logMsg << "Received " << res->body << "; response status code: " << res->status << "\n";
                retStatus = maybeRetStatus.value();
            }
            else
            {
                logMsg << "ERROR; response status code: " << res->status
                       << "; unrecognized response body: " << res->body << "\n";
                retStatus = ActionExecutionStatus::ERROR;
            }
        }

        std::cout << logMsg.str() << std::flush;
        return retStatus;
    }

    const ConfigParameter<std::string> m_host;
    const ConfigParameter<int> m_port;
    const ConfigParameter<std::string> m_executePath;
    const ConfigParameter<std::string> m_getStatusPath;

    std::list<std::string> m_inExec; // to keep track of actions in execution so we know which request type to send
};

REGISTER_ACTION_TYPE(HttpGetAction)

} // namespace bnet
} // namespace capybot
