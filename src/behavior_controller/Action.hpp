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

#include "petri_net/Token.hpp"

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

enum ActionExecutionStatus
{
    ACTION_EXEC_STATUS_COMPLETED_SUCCESS = 0,
    ACTION_EXEC_STATUS_COMPLETED_FAILURE,
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
            m_return = m_func();
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
        : tokenId(id), task(func), delayedEpochs(delay)
    {
    }
};

struct ActionExecutionResult
{
    uint64_t tokenId; // TODO: or ptr?
    ActionExecutionStatus status;
};


/// @brief Action object to be associated with a place
class Action
{
public:
    Action(ThreadPool& tp) : m_threadPool(tp) {}

    class Factory
    {
    public:
        /// List of implemented actions
        enum ActionType
        {
            ACTION_TYPE_SLEEP = 0,
        };

        static std::unique_ptr<Action> create(ThreadPool& tp, ActionType type, nlohmann::json const parameters)
        {
            std::unique_ptr<Action> action =
                std::make_unique<Action>(tp); // do we really need to deal with pointers here?
            switch (type)
            {
            case ACTION_TYPE_SLEEP: {
                action->m_actionImpl = std::make_unique<SleepAction>(parameters);
                break;
            }

            default:
                break;
            }
            return action;
        }
    };

    void executeAsync(Token::SharedPtrVec tokens)
    {
        if (!m_epochExecutions.empty())
        {
            throw LogicError("Action::executeAsync: 'getEpochResults' must be called for all 'executeAsync' calls.");
        }

        for (auto&& tokenPtr : tokens)
        {
            if (isInDelayedExecution(tokenPtr->getUniqueId())) // add unit test
                continue;

            m_epochExecutions.emplace_back(tokenPtr->getUniqueId(), m_actionImpl->createCallable(tokenPtr));
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
        virtual std::function<ActionExecutionStatus()> createCallable(Token::SharedPtr tokenPtr) = 0;
    };

    std::unique_ptr<IActionImpl> m_actionImpl{};

    class SleepAction : public IActionImpl
    {
    public:
        SleepAction(nlohmann::json const config) : m_durationMs(config.at("duration_ms").get<uint32_t>()) {}

        std::function<ActionExecutionStatus()> createCallable(Token::SharedPtr tokenPtr) override
        {
            std::ignore = tokenPtr;
            return [duration = m_durationMs]() -> ActionExecutionStatus {
                std::this_thread::sleep_for(duration);
                return ACTION_EXEC_STATUS_COMPLETED_SUCCESS;
            };
        }

    private:
        const std::chrono::milliseconds m_durationMs;
    };
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_ACTION_HPP_