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
#include "behavior_net/ConfigParameter.hpp"

#include <behavior_net/ActionRegistry.hpp>
#include <chrono>
#include <random>
#include <unordered_map>

namespace capybot
{
namespace bnet
{

/**
 * @brief This action simply holds the token for a certain amount of time
 *
 * Config parameters:
 *     "duration_ms"  [uint32_t] how long to hold the token for
 *     "failure_rate" [float][range: 0.0, 1.0][default: 0.0] rate in which the action should result in failure
 *     "error_rate"   [float][range: 0.0, 1.0][default: 0.0] rate in which the action should result in error
 */
class TimerAction : public IActionImpl
{
public:
    TimerAction(nlohmann::json const config)
        : m_durationMs(config.at("duration_ms"))
        , m_failureRate(config.contains("failure_rate") ? config.at("failure_rate") : nlohmann::json{0.f})
        , m_errorRate(config.contains("error_rate") ? config.at("error_rate") : nlohmann::json{0.f})
        , m_rd()
        , m_gen(m_rd())
    {
    }

    std::function<ActionExecutionStatus()> createCallable(Token::ConstSharedPtr token) override
    {
        float failureRate = m_failureRate.get(token);
        float errorRate = m_errorRate.get(token);
        float successRate = 1.f - failureRate - errorRate;

        std::discrete_distribution<> d({successRate, failureRate, errorRate});
        const auto result = ActionExecutionStatus::_from_integral(d(m_gen));

        const auto durationMs = m_durationMs.get(token);

        return [this, token, durationMs, result]() -> ActionExecutionStatus {
            const auto now = std::chrono::system_clock::now();

            if (m_tokenIdToFinishTime.find(token.get()) == m_tokenIdToFinishTime.end()) // new timer
            {
                m_tokenIdToFinishTime[token.get()] = now + std::chrono::milliseconds(durationMs);
            }

            if (now > m_tokenIdToFinishTime[token.get()]) // timer is done
            {
                m_tokenIdToFinishTime.erase(token.get());
                return result;
            }

            return ActionExecutionStatus::IN_PROGRESS; // timer is not done
        };
    }

private:
    const ConfigParameter<uint32_t> m_durationMs;
    const ConfigParameter<float> m_failureRate;
    const ConfigParameter<float> m_errorRate;

    std::unordered_map<const Token*, std::chrono::time_point<std::chrono::system_clock>> m_tokenIdToFinishTime;

    std::random_device m_rd;
    std::mt19937 m_gen;
};

REGISTER_ACTION_TYPE(TimerAction)

} // namespace bnet
} // namespace capybot
