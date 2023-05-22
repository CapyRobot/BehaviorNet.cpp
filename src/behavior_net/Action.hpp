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

#include "behavior_net/ThreadPool.hpp"
#include "behavior_net/Token.hpp"
#include "behavior_net/Types.hpp"

#include <list>

namespace capybot
{
namespace bnet
{

struct ActionExecutionUnit
{
    using List = std::list<ActionExecutionUnit>;

    Token::SharedPtr tokenPtr;
    ThreadPool::Task task;
    uint32_t delayedEpochs;

    ActionExecutionUnit(Token::SharedPtr const& token, std::function<ActionExecutionStatus()> func, uint32_t delay = 0)
        : tokenPtr(token)
        , task(func)
        , delayedEpochs(delay)
    {
    }
};

struct ActionExecutionResult
{
    Token::SharedPtr tokenPtr;
    ActionExecutionStatus status;
};

/// @brief  Interface for all implementations
class IActionImpl
{
public:
    virtual std::function<ActionExecutionStatus()> createCallable(Token::ConstSharedPtr token) = 0;
};

/// @brief Action object to be associated with a place
class Action
{
public:
    using UniquePtr = std::unique_ptr<Action>;

    Action(ThreadPool& tp, std::unique_ptr<IActionImpl>& impl)
        : m_threadPool(tp)
        , m_actionImpl(std::move(impl))
    {
    }

    void executeAsync(std::list<Token::SharedPtr> const& tokens)
    {
        if (!m_epochExecutions.empty())
        {
            throw LogicError("Action::executeAsync: `getEpochResults()` must be called for all 'executeAsync' calls.");
        }

        for (auto&& token : tokens)
        {
            if (isInDelayedExecution(token)) // TODO: add unit test
                continue;

            m_epochExecutions.emplace_back(token, m_actionImpl->createCallable(token));
            m_threadPool.executeAsync(m_epochExecutions.back().task);
        }
    }

    std::vector<ActionExecutionResult> getEpochResults()
    {
        std::vector<ActionExecutionResult> results;
        results.reserve(m_delayedExecutions.size() + m_epochExecutions.size());

        // let's start with the delayed ones
        {
            ActionExecutionUnit::List::iterator it = m_delayedExecutions.begin();
            while (it != m_delayedExecutions.end())
            {
                auto status = it->task.getStatus();
                if (status._value != ActionExecutionStatus::NOT_STARTED &&
                    status._value != ActionExecutionStatus::QUERRY_TIMEOUT) // execution is done
                {
                    results.push_back(ActionExecutionResult{.tokenPtr = it->tokenPtr, .status = status});
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
                if (status._value != ActionExecutionStatus::NOT_STARTED &&
                    status._value != ActionExecutionStatus::QUERRY_TIMEOUT) // execution is done
                {
                    results.push_back(ActionExecutionResult{.tokenPtr = unit.tokenPtr, .status = status});
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
    bool isInDelayedExecution(Token::ConstSharedPtr const& tokenPtr) const
    {
        const auto checkPtr = [&tokenPtr](const ActionExecutionUnit& unit) { return unit.tokenPtr == tokenPtr; };
        return std::find_if(m_delayedExecutions.begin(), m_delayedExecutions.end(), checkPtr) !=
               m_delayedExecutions.end();
    }

    ActionExecutionUnit::List m_epochExecutions{};
    ActionExecutionUnit::List m_delayedExecutions{};
    ThreadPool& m_threadPool;

    std::unique_ptr<IActionImpl> m_actionImpl{};
};

} // namespace bnet
} // namespace capybot
