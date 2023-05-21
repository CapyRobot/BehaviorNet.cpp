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

#ifndef BEHAVIOR_NET_CPP_TRANSITION_HPP_
#define BEHAVIOR_NET_CPP_TRANSITION_HPP_

#include "behavior_net/Common.hpp"
#include "behavior_net/Place.hpp"
#include "behavior_net/Token.hpp"
#include "behavior_net/Types.hpp"

#include <3rd_party/nlohmann/json.hpp>
#include <regex>
#include <vector>

namespace capybot
{
namespace bnet
{
class Transition
{
public:
    using SharedPtr = std::shared_ptr<Token>;
    using SharedPtrVec = std::vector<SharedPtr>;

    static std::vector<Transition> createTransitions(nlohmann::json const& netConfig, Place::IdMap const& places)
    {
        auto transitionConfigs = netConfig.at("transitions");

        std::vector<Transition> transitions;
        for (auto&& transitionConfig : transitionConfigs)
        {
            transitions.emplace_back(transitionConfig, places);
        }

        // TODO: ensure no repeated ids

        return transitions;
    }

    struct Arc
    {
        Place::SharedPtr place;
        ActionExecutionStatusSet resultStatusFilter{0U};
        std::optional<std::regex> contentBlockFilter; // TODO: to separate struct (+ helper methods)
    };

    Transition(nlohmann::json config, Place::IdMap const& places)
        : m_id(config.at("transition_id").get<std::string>())
    {
        if (config.contains("transition_type")) // TODO: use BETTER_ENUM for type
        {
            const auto typeStr = config.at("transition_type").get<std::string>();
            if (typeStr == "manual")
            {
                m_type = TRANSITION_TYPE_MANUAL;
            }
            else if (typeStr == "auto")
            {
                m_type = TRANSITION_TYPE_AUTO;
            }
            else
            {
                throw InvalidValueError("Transition::Transition: unknown transition type: " + typeStr);
            }
        }
        else // default TODO: warning
        {
            m_type = TRANSITION_TYPE_AUTO;
        }

        for (auto&& arcConfig : config.at("transition_arcs")) // TODO: to Arc constructor
        {
            Arc arc;
            const auto placeId = arcConfig.at("place_id").get<std::string>();
            arc.place = places.at(placeId);
            // if (arc.place == nullptr) // TODO place id does not exist within created places
            // {
            //     throw InvalidValueError("Transition::Transition: place with this id does not exist: " + placeId);
            // }

            if (arcConfig.contains("action_result_filter"))
            {
                // TODO: assert is input

                for (auto const& status : arcConfig.at("action_result_filter"))
                {
                    arc.resultStatusFilter.set(
                        ActionExecutionStatus::_from_string_nocase(status.get<std::string>().c_str()));
                }
            }
            else
            {
                arc.resultStatusFilter = 0U;
            }

            if (arcConfig.contains("token_content_filter"))
            {
                // TODO: assert is output
                arc.contentBlockFilter = std::regex(arcConfig.at("token_content_filter").get<std::string>());
            }

            const auto typeStr = arcConfig.at("type").get<std::string>();
            if (typeStr == "output")
            {
                m_outputArcs.push_back(arc);
            }
            else if (typeStr == "input")
            {
                m_inputArcs.push_back(arc);
            }
            else
            {
                throw InvalidValueError("Transition::Transition: unknown arc type: " + typeStr);
            }
        }
    }

    std::string const& getId() const { return m_id; }

    bool isManual() const { return m_type == TRANSITION_TYPE_MANUAL; }

    bool isEnabled() const
    {
        for (auto&& arc : m_inputArcs)
        {
            if (arc.place->getNumberTokensAvailable(arc.resultStatusFilter) == 0)
            {
                return false;
            }
        }
        return true;
    }

    void trigger()
    {
        if (!isEnabled())
        {
            throw LogicError("Transition::trigger: trying to trigger disabled transition.");
        }

        std::vector<Token> consumedTokens;
        for (auto&& arc : m_inputArcs)
        {
            consumedTokens.push_back(arc.place->consumeToken(arc.resultStatusFilter));
        }

        Token outToken;
        for (auto&& t : consumedTokens)
        {
            outToken.mergeContentBlocks(t);
        }

        for (auto&& arc : m_outputArcs)
        {
            if (arc.contentBlockFilter.has_value())
            {
                Token filteredToken = outToken;
                filteredToken.filterContentBlocks(arc.contentBlockFilter.value());
                arc.place->insertToken(filteredToken);
            }
            else
            {
                arc.place->insertToken(outToken);
            }
        }
    }

private:
    std::vector<Arc> m_inputArcs;
    std::vector<Arc> m_outputArcs;
    std::string m_id;

    enum TransitionType
    {
        TRANSITION_TYPE_MANUAL = 0,
        TRANSITION_TYPE_AUTO
    } m_type;
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_TRANSITION_HPP_