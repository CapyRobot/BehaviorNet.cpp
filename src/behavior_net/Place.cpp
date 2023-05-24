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

#include <behavior_net/Config.hpp>
#include <behavior_net/Place.hpp>

namespace capybot
{
namespace bnet
{

bool validatePlacesConfig(nlohmann::json const& netConfig, std::vector<std::string>& errorMessages)
{
    errorMessages.clear();

    const auto placeConfigsOpt =
        getValueAtPath<nlohmann::json const>(netConfig, {"petri_net", "places"}, errorMessages);
    if (!placeConfigsOpt.has_value())
    {
        return false;
    }

    // no repeated ids
    std::vector<std::string> ids{};
    for (auto&& placeConfig : placeConfigsOpt.value())
    {
        const auto id = getValueAtKey<std::string>(placeConfig, "place_id", errorMessages);
        if (id.has_value())
        {
            if (std::find(ids.begin(), ids.end(), id.value()) != ids.end())
            {
                errorMessages.push_back("Repeated `place_id`: " + id.value());
            }
            else
            {
                ids.push_back(id.value());
            }
        }
    }

    return errorMessages.empty();
}

REGISTER_NET_CONFIG_VALIDATOR(&validatePlacesConfig, "PlacesConfigValidator");

void Place::setAssociatedAction(ThreadPool& tp, std::string const& type, nlohmann::json const& parameters)
{
    if (m_action)
    {
        throw Exception(ExceptionType::RUNTIME_ERROR,
                        "Place::setAssociatedAction: trying to override existing action; likely a config file issue.")
            .appendMetadata("place_id", getId());
    }

    m_action = ActionRegistry::create(tp, type, parameters);
}

void Place::insertToken(Token::SharedPtr token)
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

Token::SharedPtr Place::consumeToken(ActionExecutionStatusSet resultsAccepted)
{
    if (getNumberTokensAvailable(resultsAccepted) == 0U)
    {
        throw Exception(ExceptionType::LOGIC_ERROR,
                        "Place::consumeToken: no tokens available for consumption. `getNumberTokensAvailable()` "
                        "should have been called beforehand.")
            .appendMetadata("place_id", getId())
            .appendMetadata("available tokens", getNumberTokensAvailable())
            .appendMetadata("busy tokens", getNumberTokensBusy())
            .appendMetadata("total tokens", getNumberTokensTotal());
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

void Place::executeActionAsync()
{
    if (!isPassive())
    {
        m_action->executeAsync(getTokensBusy());
    }
}

void Place::checkActionResults()
{
    if (!isPassive())
    {
        const auto actionResults = m_action->getEpochResults();

        for (auto&& result : actionResults)
        {
            if (result.status != +ActionExecutionStatus::SUCCESS && result.status != +ActionExecutionStatus::FAILURE &&
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
                throw Exception(ExceptionType::LOGIC_ERROR,
                                "Place::checkActionResults: action result token ptr does not match any busy tokens.")
                    .appendMetadata("place_id", getId())
                    .appendMetadata("busy tokens", getNumberTokensBusy());
            }
        }
    }
}

} // namespace bnet
} // namespace capybot
