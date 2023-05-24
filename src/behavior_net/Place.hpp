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
    using IdMap = std::map<std::string, SharedPtr>;

    class Factory
    {
    public:
        static IdMap createPlaces(nlohmann::json const& netConfig)
        {
            auto placeConfigs = netConfig.at("places");
            IdMap placePtrs;
            for (auto&& placeConfig : placeConfigs)
            {
                placePtrs.emplace(placeConfig.at("place_id").get<std::string>(), std::make_shared<Place>(placeConfig));
            }
            return placePtrs;
        }

        static void createActions(ThreadPool& tp, nlohmann::json const actionsConfig, IdMap& places)
        {
            for (auto&& config : actionsConfig)
            {
                places
                    .at(config["place_id"])
                    ->setAssociatedAction(tp, config["type"], config["params"]);
            }
        }
    };

    Place(nlohmann::json config)
        : m_id(config.at("place_id").get<std::string>())
        , m_action(nullptr)
    {
    }

    void setAssociatedAction(ThreadPool& tp, std::string const& type, nlohmann::json const& parameters);
    void insertToken(Token::SharedPtr token);
    Token::SharedPtr consumeToken(ActionExecutionStatusSet resultsAccepted = 0U);
    void executeActionAsync();
    void checkActionResults();

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
