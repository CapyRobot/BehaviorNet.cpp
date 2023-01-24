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

#ifndef BEHAVIOR_NET_CPP_BEHAVIOR_NET_HPP_
#define BEHAVIOR_NET_CPP_BEHAVIOR_NET_HPP_

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

class INet
{
public:
    using Marking = std::unordered_map<std::string, uint32_t>;

    // virtual void run() = 0;
    // virtual void pause() = 0;
    // virtual void stop() = 0;

    virtual void addToken(Token::SharedPtr const &newToken, std::string_view placeId) = 0;

    // struct Marking
    // {
    //     std::vector<std::pair<std::string, uint32_t>> places;
    //     std::vector<std::pair<std::ve, uint32_t>> places;
    // };
    // virtual Marking getCurrentState() const = 0;

    virtual void prettyPrintState() const = 0;

    virtual Marking getCurrentMarking() const = 0;

    virtual void triggerManualTransition(std::string_view const &id) = 0;

    // virtual BlackboardPtr getBlackboardPtr() = 0;
};

std::unique_ptr<INet> create(BehaviorNetConfig const &config);

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_BEHAVIOR_NET_HPP_