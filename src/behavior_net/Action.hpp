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

#ifndef BEHAVIOR_NET_CPP_ACTION_HPP_
#define BEHAVIOR_NET_CPP_ACTION_HPP_ // TODO: this pattern may create conflicts. Change to project_dir_filename
                                     // (currently missing dir)

#include "behavior_net/Token.hpp"

#include "3rd_party/cpp-httplib/httplib.h"
#include "3rd_party/taskflow/taskflow.hpp"

#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>

namespace capybot
{
namespace bnet
{

namespace helpers
{
inline std::string getContentBetweenChars(const std::string& str, char start, char end)
{
    size_t startPos = str.find(start);
    if (startPos == std::string::npos)
        return "";

    size_t endPos = str.find(end, startPos + 1);
    if (endPos == std::string::npos)
        return "";

    return str.substr(startPos + 1, endPos - startPos - 1);
}

inline std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));
    return tokens;
}
} // namespace helpers

enum ActionExecutionStatus
{
    ACTION_EXEC_STATUS_COMPLETED_SUCCESS = 0,
    ACTION_EXEC_STATUS_COMPLETED_FAILURE,
    ACTION_EXEC_STATUS_COMPLETED_ERROR,
    ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS, // add COMPLETED
    ACTION_EXEC_STATUS_QUERRY_TIMEOUT,
    ACTION_EXEC_STATUS_NOT_STARTED,
};

/**
 * TODO: implementation is just a quick prototype,  Factual implementation should be cleaned up and separated into
 * another cpp file.
 *
 */
class ThreadPool
{
public:
    class Task
    {
    public:
        Task(std::function<ActionExecutionStatus()> func) : m_func(func), m_started(false), m_done(false) {}

        /// Execute task synchronously - used by the TP
        void executeSync()
        {
            {
                std::lock_guard<std::mutex> lk(m_mtx); // probably unnecessary
                m_started = true;
                m_done = false;
            }
            m_return = m_func(); // TODO: try? it can be user provided code
            {
                std::lock_guard<std::mutex> lk(m_mtx);
                m_done = true;
            }
            m_waitCondition.notify_all();
        }

        /// Get return value after completion
        /// @throws ... if the task was not started
        ActionExecutionStatus getStatus(uint32_t timeoutUs = 0U) const
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            if (!m_started)
                return ACTION_EXEC_STATUS_NOT_STARTED;
            if (m_done)
                return m_return;

            if (timeoutUs)
            {
                if (m_waitCondition.wait_for(lk, std::chrono::microseconds(timeoutUs), [this] { return m_done; }))
                    return m_return;
            }
            return ACTION_EXEC_STATUS_QUERRY_TIMEOUT;
        }

    private:
        const std::function<ActionExecutionStatus()> m_func;
        ActionExecutionStatus m_return;
        bool m_started;
        bool m_done;
        mutable std::condition_variable m_waitCondition;
        mutable std::mutex m_mtx;
    };

    ThreadPool(uint32_t numberOfThreads = std::thread::hardware_concurrency()) : m_executor(numberOfThreads) {}
    ~ThreadPool() {}

    /// @brief add task to the thread pool queue
    void executeAsync(Task& task)
    {
        m_executor.silent_async([&task] { task.executeSync(); });
    }

private:
    tf::Executor m_executor;
};

struct ActionExecutionUnit
{
    using List = std::list<ActionExecutionUnit>;

    uint64_t tokenId;
    ThreadPool::Task task;
    uint32_t delayedEpochs;

    ActionExecutionUnit(uint64_t id, std::function<ActionExecutionStatus()> func, uint32_t delay = 0)
        : tokenId(id)
        , task(func)
        , delayedEpochs(delay)
    {
    }
};

struct ActionExecutionResult
{
    uint64_t tokenId; // TODO: ptr or iterator
    ActionExecutionStatus status;
};

/// @brief Action object to be associated with a place
class Action
{
public:
    using UniquePtr = std::unique_ptr<Action>;

    Action(ThreadPool& tp) : m_threadPool(tp) {}

    class Factory
    {
    public:
        /// List of implemented actions
        enum ActionType
        {
            ACTION_TYPE_SLEEP = 0,
            ACTION_TYPE_HTTP_GET
        };

        static ActionType typeFromStr(std::string const& typeStr)
        {
            static const std::unordered_map<std::string, ActionType> s_typeStrMap = {
                {"ACTION_TYPE_SLEEP", ACTION_TYPE_SLEEP}, {"ACTION_TYPE_HTTP_GET", ACTION_TYPE_HTTP_GET}};
            return s_typeStrMap.at(typeStr);
        }

        static UniquePtr create(ThreadPool& tp, ActionType type, nlohmann::json const parameters)
        {
            UniquePtr action = std::make_unique<Action>(tp); // do we really need to deal with pointers here?
            switch (type)                                    // TODO: to tag specializaton patter
            {
            case ACTION_TYPE_SLEEP: {
                action->m_actionImpl = std::make_unique<SleepAction>(parameters);
                break;
            }
            case ACTION_TYPE_HTTP_GET: {
                action->m_actionImpl = std::make_unique<HttpGetAction>(parameters);
                break;
            }
            default:
                break;
            }
            return action;
        }

        static std::map<std::string, UniquePtr> createActionMap(ThreadPool& tp, nlohmann::json const actionsConfig)
        {
            std::map<std::string, UniquePtr> actionMap;
            for (auto&& config : actionsConfig)
            {
                actionMap.emplace(config["place"], create(tp, typeFromStr(config["type"]), config["params"]));
            }
            return actionMap;
        }
    };

    void executeAsync(std::list<Token> const& tokens)
    {
        if (!m_epochExecutions.empty())
        {
            throw LogicError("Action::executeAsync: 'getEpochResults' must be called for all 'executeAsync' calls.");
        }

        for (auto&& token : tokens)
        {
            if (isInDelayedExecution(token.getUniqueId())) // add unit test
                continue;

            m_epochExecutions.emplace_back(token.getUniqueId(), m_actionImpl->createCallable(token));
            m_threadPool.executeAsync(m_epochExecutions.back().task);
        }
    }

    std::vector<ActionExecutionResult> getEpochResults() // TODO: add wait until logic
    {
        std::vector<ActionExecutionResult> results;
        results.reserve(m_delayedExecutions.size() + m_epochExecutions.size());

        // let's start with the delayed ones
        {
            ActionExecutionUnit::List::iterator it = m_delayedExecutions.begin();
            while (it != m_delayedExecutions.end())
            {
                auto status = it->task.getStatus();
                if (status != ACTION_EXEC_STATUS_NOT_STARTED &&
                    status != ACTION_EXEC_STATUS_QUERRY_TIMEOUT) // execution is done
                {
                    results.push_back(ActionExecutionResult{.tokenId = it->tokenId, .status = status});
                    m_delayedExecutions.erase(it++);
                }
                else
                {
                    it->delayedEpochs++;
                    ++it;
                }
            }
        }

        // now, this epochs executions
        {
            while (!m_epochExecutions.empty())
            {
                auto& unit = m_epochExecutions.front();
                auto status = unit.task.getStatus();
                if (status != ACTION_EXEC_STATUS_NOT_STARTED &&
                    status != ACTION_EXEC_STATUS_QUERRY_TIMEOUT) // execution is done
                {
                    results.push_back(ActionExecutionResult{.tokenId = unit.tokenId, .status = status});
                    m_epochExecutions.pop_front();
                }
                else
                {
                    // move first element to delayed executionslist
                    ++unit.delayedEpochs;
                    m_delayedExecutions.splice(m_delayedExecutions.end(), m_epochExecutions, m_epochExecutions.begin());
                }
            }
        }

        return results;
    }

    uint32_t getNumberDelayedTasks() const { return m_delayedExecutions.size(); }

private:
    bool isInDelayedExecution(uint64_t tokenId) const
    {
        const auto checkId = [&tokenId](const ActionExecutionUnit& unit) { return unit.tokenId == tokenId; };
        return std::find_if(m_delayedExecutions.begin(), m_delayedExecutions.end(), checkId) !=
               m_delayedExecutions.end();
    }

    ActionExecutionUnit::List m_epochExecutions{};
    ActionExecutionUnit::List m_delayedExecutions{};
    ThreadPool& m_threadPool;

    /// @brief  Interface for all implementations
    class IActionImpl
    {
    public:
        virtual std::function<ActionExecutionStatus()> createCallable(Token const& token) = 0;
    };

    std::unique_ptr<IActionImpl> m_actionImpl{};

    template <typename T>
    static T getConfigParam(nlohmann::json const& configParam, Token const& token) // TODO: to container
    {
        auto const str = configParam.get<std::string>();
        if (str.find("@token") != std::string::npos)
        {
            const auto path = helpers::split(helpers::getContentBetweenChars(str, '{', '}'), '.');
            const auto contentBlockKey = path.at(0);
            nlohmann::json data = token.getContent(contentBlockKey);
            for (auto it = path.begin() + 1; it != path.end(); ++it)
            {
                data = data.at(*it);
            }
            return data.get<T>();
        }
        else
        {
            return configParam.get<T>();
        }
    }

    class SleepAction : public IActionImpl
    {
    public:
        SleepAction(nlohmann::json const config) : m_durationMs(config.at("duration_ms")) {}

        std::function<ActionExecutionStatus()> createCallable(Token const& token) override
        {
            uint32_t durationMs = getConfigParam<uint32_t>(m_durationMs, token);
            return [durationMs]() -> ActionExecutionStatus {
                std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
                return ACTION_EXEC_STATUS_COMPLETED_SUCCESS;
            };
        }

    private:
        // keeping a json object and not a number because the config paramerter may be dependent on the token, i.e.,
        // '@token{...}'
        const nlohmann::json m_durationMs;
    };

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

        std::function<ActionExecutionStatus()> createCallable(Token const& token) override
        {
            return [&token, this]() -> ActionExecutionStatus {
                auto host = getConfigParam<std::string>(m_host, token);
                auto port = getConfigParam<int>(m_port, token);
                auto executePath = getConfigParam<std::string>(m_executePath, token);
                auto getStatusPath = getConfigParam<std::string>(m_getStatusPath, token);

                auto const actionId = host + std::to_string(port) + executePath;
                bool const isInExecution = std::find(m_inExec.begin(), m_inExec.end(), actionId) != m_inExec.end();

                ActionExecutionStatus retStatus;
                if (isInExecution)
                {
                    retStatus = request(host, port, getStatusPath);
                    if (retStatus != ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS)
                    {
                        m_inExec.remove(actionId);
                    }
                }
                else
                {
                    retStatus = request(host, port, executePath);
                    if (retStatus == ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS)
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

            ActionExecutionStatus retStatus;
            if (!res)
            {
                auto err = res.error();
                logMsg << "ERROR; HTTP error: " << httplib::to_string(err) << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_ERROR;
            }
            else if (res->status < 200 && res->status >= 300)
            {
                logMsg << "ERROR; response status code: " << res->status << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_ERROR;
            }
            else if (res->body == "ACTION_EXEC_STATUS_COMPLETED_SUCCESS")
            {
                logMsg << "Received SUCCESS; response status code: " << res->status << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_SUCCESS;
            }
            else if (res->body == "ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS")
            {
                logMsg << "Received IN_PROGRESS; response status code: " << res->status << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_IN_PROGRESS;
            }
            else if (res->body == "ACTION_EXEC_STATUS_COMPLETED_FAILURE")
            {
                logMsg << "Received FAILURE; response status code: " << res->status << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_FAILURE;
            }
            else if (res->body == "ACTION_EXEC_STATUS_COMPLETED_ERROR")
            {
                logMsg << "Received ERROR; response status code: " << res->status << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_ERROR;
            }
            else
            {
                logMsg << "ERROR; response status code: " << res->status
                       << "; unrecognized response body: " << res->body << "\n";
                retStatus = ACTION_EXEC_STATUS_COMPLETED_ERROR;
            }

            std::cout << logMsg.str() << std::flush;
            return retStatus;
        }

        // keeping a json object and not a number because the config paramerter may be dependent on the token, i.e.,
        // '@token{...}'
        const nlohmann::json m_host;
        const nlohmann::json m_port;
        const nlohmann::json m_executePath;
        const nlohmann::json m_getStatusPath;

        std::list<std::string> m_inExec;
    };
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_ACTION_HPP_