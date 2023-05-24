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

#include <3rd_party/taskflow/taskflow.hpp>
#include <behavior_net/Types.hpp>

#include <condition_variable>
#include <functional>
#include <mutex>

namespace capybot
{
namespace bnet
{

/**
 * TODO: implementation is just a quick prototype, actual implementation should be cleaned up
 *
 */
class ThreadPool
{
public:
    class Task
    {
    public:
        Task(std::function<ActionExecutionStatus()> func)
            : m_func(func)
            , m_return(ActionExecutionStatus::NOT_STARTED)
            , m_started(false)
            , m_done(false)
        {
        }

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
                return ActionExecutionStatus::NOT_STARTED;
            if (m_done)
                return m_return;

            if (timeoutUs)
            {
                if (m_waitCondition.wait_for(lk, std::chrono::microseconds(timeoutUs), [this] { return m_done; }))
                    return m_return;
            }
            return ActionExecutionStatus::QUERRY_TIMEOUT;
        }

    private:
        const std::function<ActionExecutionStatus()> m_func;
        ActionExecutionStatus m_return;
        bool m_started;
        bool m_done;
        mutable std::condition_variable m_waitCondition;
        mutable std::mutex m_mtx;
    };

    ThreadPool(uint32_t numberOfThreads = std::thread::hardware_concurrency())
        : m_executor(numberOfThreads)
    {
    }
    ~ThreadPool() {}

    /// @brief add task to the thread pool queue
    void executeAsync(Task& task)
    {
        m_executor.silent_async([&task] { task.executeSync(); });
    }

private:
    tf::Executor m_executor;
};

} // namespace bnet
} // namespace capybot
