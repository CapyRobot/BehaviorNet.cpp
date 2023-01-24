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

#ifndef BEHAVIOR_NET_CPP_PLACE_HPP_
#define BEHAVIOR_NET_CPP_PLACE_HPP_

#include "behavior_net/Common.hpp"
#include "behavior_net/Config.hpp"
#include "behavior_net/Token.hpp"

#include <3rd_party/nlohmann/json.hpp>
#include <deque>
#include <unordered_map>

namespace capybot
{
namespace bnet
{

class Place
{
public:
    using SharedPtr = std::shared_ptr<Place>;
    using SharedPtrVec = std::vector<SharedPtr>;

    static std::vector<std::shared_ptr<Place>> createPlaces(BehaviorNetConfig const &netConfig)
    {
        auto placeConfigs = netConfig.get().at("places");

        std::vector<std::shared_ptr<Place>> placePtrs;
        for (auto &&placeConfig : placeConfigs)
        {
            placePtrs.emplace_back(std::make_shared<Place>(placeConfig));
        }

        // TODO: ensure no repeated ids

        return placePtrs;
    }

    Place(nlohmann::json config) : m_id(config.at("place_id").get<std::string>()) {}

    std::string const &getId() const { return m_id; }

    void insertToken(Token::SharedPtr tokenPtr) { m_tokens.push_back(tokenPtr); }

    Token::SharedPtr consumeToken()
    {
        auto token = m_tokens.front();
        m_tokens.pop_front();
        return token;
    }

    uint32_t getNumberTokens() const { return m_tokens.size(); }

private:
    std::string m_id;
    std::deque<Token::SharedPtr> m_tokens;
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_PLACE_HPP_