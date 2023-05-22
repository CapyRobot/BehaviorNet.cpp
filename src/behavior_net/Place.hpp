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

#include "behavior_net/ActionRegistry.hpp"
#include "behavior_net/Common.hpp"
#include "behavior_net/Token.hpp"

#include <3rd_party/nlohmann/json.hpp>
#include <list>
#include <optional>
#include <unordered_map>

namespace capybot
{
namespace bnet
{

class Place
{
public:
    using SharedPtr = std::shared_ptr<Place>;
    using IdMap = std::map<std::string, SharedPtr>; // TODO: why shared_ptr?

    class Factory
    {
    public:
        static IdMap createPlaces(nlohmann::json const& netConfig)
        {
            auto placeConfigs = netConfig.at("places");
            IdMap placePtrs;
            for (auto&& placeConfig : placeConfigs)
            {
                // TODO: ensure no repeated ids
                placePtrs.emplace(placeConfig.at("place_id").get<std::string>(), std::make_shared<Place>(placeConfig));
            }
            return placePtrs;
        }

        static void createActions(ThreadPool& tp, nlohmann::json const actionsConfig, IdMap& places)
        {
            for (auto&& config : actionsConfig)
            {
                places
                    .at(config["place"]) // TODO: place -> place_id
                    ->setAssociatedAction(tp, config["type"], config["params"]);
            }
        }
    };

    Place(nlohmann::json config)
        : m_id(config.at("place_id").get<std::string>())
        , m_action(nullptr)
    {
    }

    void setAssociatedAction(ThreadPool& tp, std::string const& type, nlohmann::json const& parameters)
    {
        if (m_action)
        {
            throw RuntimeError("Place::setAssociatedAction: trying to override existing action;"
                               " likely a config file issue.");
        }

        m_action = ActionRegistry::create(tp, type, parameters);
    }

    void insertToken(Token::SharedPtr token)
    {
        if (isPassive())
        {
            m_tokensAvailable.push_back({token, ActionExecutionStatus::SUCCESS});
        }
        else
        {
            m_tokensBusy.push_back(token);
        }
    }

    Token::SharedPtr consumeToken(ActionExecutionStatusSet resultsAccepted = 0U)
    {
        if (getNumberTokensAvailable(resultsAccepted) == 0U)
        {
            throw LogicError("Place::consumeToken: no tokens available for consumption. `getNumberTokensAvailable()` "
                             "should have been called beforehand.");
        }

        Token::SharedPtr token{};
        if (resultsAccepted.any())
        {
            for (auto it = m_tokensAvailable.begin(); it != m_tokensAvailable.end(); it++)
            {
                if (resultsAccepted.test(it->status))
                {
                    token = it->tokenPtr;
                    m_tokensAvailable.erase(it);
                    break;
                }
            }
        }
        else
        {
            token = m_tokensAvailable.front().tokenPtr;
            m_tokensAvailable.pop_front();
        }
        return token;
    }

    void executeActionAsync()
    {
        if (!isPassive())
        {
            m_action->executeAsync(getTokensBusy());
        }
    }

    void checkActionResults()
    {
        if (!isPassive())
        {
            const auto actionResults = m_action->getEpochResults();

            for (auto&& result : actionResults)
            {
                if (result.status != +ActionExecutionStatus::SUCCESS &&
                    result.status != +ActionExecutionStatus::FAILURE &&
                    result.status != +ActionExecutionStatus::ERROR) // not completed
                {
                    continue;
                }

                auto it = std::find(m_tokensBusy.begin(), m_tokensBusy.end(), result.tokenPtr);
                if (it != m_tokensBusy.end())
                {
                    m_tokensBusy.erase(it);
                    m_tokensAvailable.push_back(result);
                }
                else
                {
                    throw LogicError("Place::checkActionResults: action result id does not match any busy tokens.");
                }
            }
        }
    }

    bool isPassive() const { return m_action == nullptr; }
    std::string const& getId() const { return m_id; }

    uint32_t getNumberTokensBusy() const { return m_tokensBusy.size(); }
    uint32_t getNumberTokensTotal() const { return m_tokensBusy.size() + m_tokensAvailable.size(); }
    uint32_t getNumberTokensAvailable(ActionExecutionStatusSet status = 0U) const
    {
        if (status.any())
        {
            return std::count_if(m_tokensAvailable.begin(), m_tokensAvailable.end(),
                                 [&status](ActionExecutionResult const& s) { return status.test(s.status); });
        }
        return m_tokensAvailable.size();
    }

    std::list<Token::SharedPtr> const& getTokensBusy() const { return m_tokensBusy; }

private:
    std::string m_id;
    Action::UniquePtr m_action;

    std::list<ActionExecutionResult> m_tokensAvailable; // ready to be consumed

    std::list<Token::SharedPtr> m_tokensBusy; // either in action exec or waiting for exec
};

} // namespace bnet
} // namespace capybot
