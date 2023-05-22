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

#include "behavior_net/Config.hpp"
#include "behavior_net/Place.hpp"
#include "behavior_net/Token.hpp"
#include "behavior_net/Transition.hpp"

#include <memory>
#include <string_view>

namespace capybot
{
namespace bnet
{

class PetriNet
{
public:
    static std::unique_ptr<PetriNet> create(NetConfig const& config)
    {
        return std::make_unique<PetriNet>(config.get().at("petri_net"));
    }

    PetriNet(nlohmann::json const& config)
        : m_config(config)
    {
        m_places = Place::createPlaces(config);
        m_transitions = Transition::createTransitions(config, m_places);

        // m_incidenceMatrixPlus.reset(m_transitions.size(), m_places.size(), 0U);
        // m_incidenceMatrixMinus.reset(m_transitions.size(), m_places.size(), 0U);
        // for (auto&& t : m_transitions)
        // {
        // }
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
            throw LogicError("PetriNet::addToken: place with this id does not exist: " + std::string(placeId));
        }
    }

    void prettyPrintState() const
    {
        std::cout << "Marking:" << std::endl;
        for (auto&& [id, placePtr] : m_places)
        {
            std::cout << "\tPlace " << id << ": " << placePtr->getNumberTokensTotal() << std::endl;
        }
    }

    void triggerTransition(std::string_view const& id, bool assertIsManual = false)
    {
        bool exists{false};
        for (auto&& transition : m_transitions)
        {
            if (transition.getId() == id)
            {
                if (assertIsManual && !transition.isManual())
                {
                    throw LogicError("PetriNet::triggerTransition: trying to manually trigger an auto transition: " +
                                     std::string(id));
                }

                exists = true;
                transition.trigger();
            }
        }
        if (!exists)
        {
            throw LogicError("PetriNet::triggerTransition: transition with this id does not exist: " + std::string(id));
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

    Place::IdMap m_places; // why shared ptr?
    std::vector<Transition> m_transitions;

    // TODO: not needed so far
    // template <typename T>
    // struct Matrix
    // {
    //     void reset(uint32_t rows, uint32_t cols, T initValue = {})
    //     {
    //         m_rows = rows;
    //         m_cols = cols;
    //         m_data = std::vector<T>(rows * cols, initValue);
    //     }

    //     T at(uint32_t row, uint32_t col) const { return m_data.at(col + row * m_cols); }
    //     T& at(uint32_t row, uint32_t col) { return m_data[col + row * m_cols]; }

    // private:
    //     uint32_t m_rows{0}, m_cols{0};
    //     std::vector<T> m_data{};
    // };
    // Matrix<uint32_t> m_incidenceMatrixPlus;
    // Matrix<uint32_t> m_incidenceMatrixMinus;
};

} // namespace bnet
} // namespace capybot
