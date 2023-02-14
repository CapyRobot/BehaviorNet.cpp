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

#ifndef BEHAVIOR_NET_CPP_TOKEN_HPP_
#define BEHAVIOR_NET_CPP_TOKEN_HPP_

#include <3rd_party/nlohmann/json.hpp>
#include <iostream>
#include <regex>
#include <unordered_map>

#include "behavior_net/Common.hpp"

namespace capybot
{
namespace bnet
{

class Token
{
public:
    static constexpr uint64_t INVALID_TOKEN_ID{0UL};

    Token() : m_uniqueId(generateUniqueId()) {}
    ~Token() = default;
    Token& operator=(const Token& other) { m_contentBlocks = other.m_contentBlocks; } // keep same unique id

    bool hasKey(std::string const& key) const { return m_contentBlocks.find(key) != m_contentBlocks.end(); }

    nlohmann::json getContent(std::string const& key) const
    {
        if (!hasKey(key))
        {
            throw RuntimeError("Token::getContent: token does not contain a block for key: " + key);
        }
        return m_contentBlocks.at(key);
    }

    void addContentBlock(std::string const& key, nlohmann::json blockContent)
    {
        const auto [it, success] = m_contentBlocks.insert({key, blockContent});
        if (!success)
        {
            throw RuntimeError("Token::addContentBlock: token already has a block for key: " + key);
        }
    }

    void mergeContentBlocks(Token const& token)
    {
        for (auto block : token.m_contentBlocks)
        {
            const auto [it, success] = m_contentBlocks.insert(block);
            if (!success)
            {
                throw RuntimeError("Token::mergeContentBlocks: token already has a block for key: " + block.first);
            }
        }
    }

    void filterContentBlocks(std::regex const& keyRegex)
    {
        decltype(m_contentBlocks) matchedBlocks;
        for (auto it = m_contentBlocks.begin(); it != m_contentBlocks.end(); ++it)
        {
            if (std::regex_match(it->first, keyRegex))
            {
                matchedBlocks.insert(*it);
            }
        }
        m_contentBlocks.swap(matchedBlocks);
    }

    auto getUniqueId() const { return m_uniqueId; }
    auto getCurrentPlace() const { return m_currentPlaceId; }
    void setCurrentPlace(std::string const& placeId) { m_currentPlaceId = placeId; }

private:
    static uint64_t generateUniqueId()
    {
        static uint64_t s_uniqueIdCounter{0UL};
        ++s_uniqueIdCounter;
        if (s_uniqueIdCounter == INVALID_TOKEN_ID)
            ++s_uniqueIdCounter;
        return s_uniqueIdCounter;
    }

    uint64_t m_uniqueId;
    std::string m_currentPlaceId;
    std::unordered_map<std::string, nlohmann::json> m_contentBlocks;
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_TOKEN_HPP_