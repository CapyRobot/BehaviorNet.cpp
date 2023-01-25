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

#include "petri_net/PetriNet.hpp"

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace capybot
{
namespace bnet
{

class PetriNetImpl : public IPetriNet
{
public:
    PetriNetImpl(PetriNetConfig const &config)
    {
        m_places = Place::createPlaces(config);
        m_transitions = Transition::createTransitions(config, m_places);
    }

    // void run() override { throw "not supported - manual exec only"; }
    // void pause() override { throw "not supported - manual exec only"; }
    // void stop() override { throw "not supported - manual exec only"; }

    void addToken(Token::SharedPtr const &newToken, std::string_view placeId) override
    {
        bool placeExists{false};
        for (auto &&placePtr : m_places)
        {
            if (placePtr->getId() == placeId)
            {
                placeExists = true;
                placePtr->insertToken(newToken);
            }
        }
        if (!placeExists)
        {
            throw LogicError("PetriNetImpl::addToken: place with this id does not exist: " + std::string(placeId));
        }
    }

    // Marking getCurrentState() const override {}

    void prettyPrintState() const override
    {
        std::cout << "Marking:" << std::endl;
        for (auto &&place : m_places)
        {
            std::cout << "\tPlace " << place->getId() << ": " << place->getNumberTokens() << std::endl;
        }
    }

    Marking getCurrentMarking() const override
    {
        Marking m;
        for (auto &&p : m_places)
        {
            m.insert({p->getId(), p->getNumberTokens()});
        }
        return m;
    }

    void triggerManualTransition(std::string_view const &id) override
    {
        bool exists{false};
        for (auto &&transition : m_transitions)
        {
            if (transition.getId() == id)
            {
                exists = true;
                transition.trigger();
            }
        }
        if (!exists)
        {
            throw LogicError("PetriNetImpl::triggerManualTransition: transition with this id does not exist: " +
                             std::string(id));
        }
    }

    // BlackboardPtr getBlackboardPtr() override;

private:
    std::vector<std::shared_ptr<Place>> m_places;
    std::vector<Transition> m_transitions;
};

std::unique_ptr<IPetriNet> create(PetriNetConfig const &config)
{
    return std::make_unique<PetriNetImpl>(config);
}

} // namespace bnet
} // namespace capybot
