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

#include "petri_net/Common.hpp"
#include "petri_net/Config.hpp"
#include "petri_net/Place.hpp"
#include "petri_net/Token.hpp"

#include <3rd_party/nlohmann/json.hpp>
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

    static std::vector<Transition> createTransitions(PetriNetConfig const &netConfig, Place::SharedPtrVec places)
    {
        auto transitionConfigs = netConfig.get().at("transitions");

        std::vector<Transition> transitions;
        for (auto &&transitionConfig : transitionConfigs)
        {
            transitions.emplace_back(transitionConfig, places);
        }

        // TODO: ensure no repeated ids

        return transitions;
    }

    struct Arc
    {
        Place::SharedPtr place;
        enum ArcType
        {
            ARC_TYPE_PLACE_TO_TRANSITION = 0,
            ARC_TYPE_TRANSITION_TO_PLACE
        } type;
    };

    Transition(nlohmann::json config, Place::SharedPtrVec const &places) // TODO: places to map
        : m_id(config.at("transition_id").get<std::string>())
    {
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

        for (auto &&arcConfig : config.at("transition_arcs")) // TODO: to Arc constructor
        {
            Arc arc;
            const auto placeId = arcConfig.at("place_id").get<std::string>();
            for (auto &&placePtr : places)
            {
                if (placeId == placePtr->getId())
                {
                    arc.place = placePtr;
                }
            }
            if (arc.place == nullptr) // place id does not exist within created places
            {
                throw InvalidValueError("Transition::Transition: place with this id does not exist: " + placeId);
            }

            const auto typeStr = arcConfig.at("arc_type").get<std::string>();
            if (typeStr == "output")
            {
                arc.type = Arc::ARC_TYPE_TRANSITION_TO_PLACE;
                m_outputArcs.push_back(arc);
            }
            else if (typeStr == "input")
            {
                arc.type = Arc::ARC_TYPE_PLACE_TO_TRANSITION;
                m_inputArcs.push_back(arc);
            }
            else
            {
                throw InvalidValueError("Transition::Transition: unknown arc type: " + typeStr);
            }
        }
    }

    std::string const &getId() const { return m_id; }

    bool isEnabled() const
    {
        for (auto &&arc : m_inputArcs)
        {
            if (arc.place->getNumberTokens() == 0)
            {
                return false;
            }
        }
        for (auto &&arc : m_outputArcs)
        {
            // TODO: at capacity?
        }
        return true;
    }

    void trigger()
    {
        if (!isEnabled())
        {
            throw LogicError("Transition::trigger: trying to trigger disabled transition.");
        }

        Token::SharedPtrVec consumedTokens;
        for (auto &&arc : m_inputArcs)
        {
            consumedTokens.push_back(arc.place->consumeToken());
        }

        Token::SharedPtr outToken = std::make_shared<Token>();
        for (auto &&t : consumedTokens)
        {
            outToken->mergeContentBlocks(*t);
        }

        for (auto &&arc : m_outputArcs)
        {
            arc.place->insertToken(outToken);
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