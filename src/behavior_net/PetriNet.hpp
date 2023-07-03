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

#include <behavior_net/Config.hpp>
#include <behavior_net/Place.hpp>
#include <behavior_net/Token.hpp>
#include <behavior_net/Transition.hpp>
#include <behavior_net/Types.hpp>

#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>

namespace capybot
{
namespace bnet
{

class PetriNet
{
    static constexpr const char* MODULE_TAG{"PetriNet"};

public:
    static std::unique_ptr<PetriNet> create(NetConfig const& config)
    {
        return std::make_unique<PetriNet>(config.get().at("petri_net"));
    }

    PetriNet(nlohmann::json const& config)
        : m_config(config)
    {
        m_places = Place::Factory::createPlaces(config);
        m_transitions = Transition::Factory::createTransitions(config, m_places);
    }

    /// @param newToken token to be added; will be moved so a token cannot be added more than once as tokens within the
    /// net must be unique
    void addToken(Token::UniquePtr& newToken, std::string_view placeId)
    {
        THROW_ON_NULLPTR(newToken, "PetriNet::addToken");

        bool placeExists{false};
        for (auto&& [id, placePtr] : m_places)
        {
            if (id == placeId)
            {
                placeExists = true;
                placePtr->insertToken(std::move(newToken));
            }
        }
        if (!placeExists)
        {
            throw Exception(ExceptionType::RUNTIME_ERROR, "PetriNet::addToken: place with this id does not exist.")
                .appendMetadata("place_id", placeId);
        }
    }

    void prettyPrintState() const
    {
        std::size_t max_id_size = 10;
        for (auto&& [id, _] : m_places)
        {
            max_id_size = std::max(max_id_size, id.size());
        }

        /*
          Place ID     Total Available   Success     Error   Failure
        ------------------------------------------------------------
                 A         3         3         3         0         0
                 B         1         1         1         0         0
                 C         1         1         0         1         0
                 D         1         1         1         0         0
        */

        std::stringstream ss;
        ss << "Marking:\n\n";
        ss << "\t" << std::setfill(' ') << std::setw(max_id_size) << "Place ID" << std::setw(10) << "Total"
           << std::setw(10) << "Available" << std::setw(10) << "Success" << std::setw(10) << "Error" << std::setw(10)
           << "Failure"
           << "\n";
        ss << "\t" << std::setfill('-') << std::setw(max_id_size + 50) << "-"
           << "\n";
        for (auto&& [id, placePtr] : m_places)
        {
            ss << "\t" << std::setfill(' ') << std::setw(max_id_size) << id << std::setw(10)
               << placePtr->getNumberTokensTotal() << std::setw(10) << placePtr->getNumberTokensAvailable()
               << std::setw(10) << placePtr->getNumberTokensAvailable(1 << ActionExecutionStatus::SUCCESS)
               << std::setw(10) << placePtr->getNumberTokensAvailable(1 << ActionExecutionStatus::ERROR)
               << std::setw(10) << placePtr->getNumberTokensAvailable(1 << ActionExecutionStatus::FAILURE) << "\n";
        }
        ss << "\n";
        LOG(DEBUG) << ss.str();
    }

    void triggerTransition(std::string_view const& id, bool assertIsManual = false)
    {
        LOG(DEBUG) << "triggerTransition @ " << id << "; " << (assertIsManual ? "manual" : "auto") << log::endl;

        bool exists{false};
        for (auto&& transition : m_transitions)
        {
            if (transition.getId() == id)
            {
                if (assertIsManual && !transition.isManual())
                {
                    throw Exception(ExceptionType::RUNTIME_ERROR,
                                    "PetriNet::triggerTransition: trying to manually trigger an auto transition.")
                        .appendMetadata("id", id);
                }

                exists = true;
                transition.trigger();
            }
        }
        if (!exists)
        {
            throw Exception(ExceptionType::RUNTIME_ERROR,
                            "PetriNet::triggerTransition: transition with this id does not exist.")
                .appendMetadata("id", id);
        }
    }

    auto const& getTransitions() const { return m_transitions; }
    auto const& getPlaces() const { return m_places; }
    auto& getTransitions() { return m_transitions; }
    auto& getPlaces() { return m_places; }

    nlohmann::json getMarking() const
    {
        nlohmann::json m;
        m["config"] = m_config; // TODO: ??
        m["marking"] = {};
        for (auto&& [id, placePtr] : m_places)
        {
            m["marking"][id] = placePtr->getNumberTokensTotal();
        }
        return m;
    }

private:
    nlohmann::json m_config;

    Place::IdMap m_places;
    std::vector<Transition> m_transitions;
};

} // namespace bnet
} // namespace capybot
