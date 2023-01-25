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
#include <unordered_map>

#include "petri_net/Common.hpp"

namespace capybot
{
namespace bnet
{

class Token
{
public:
    using SharedPtr = std::shared_ptr<Token>;
    using SharedPtrVec = std::vector<SharedPtr>;

    bool hasKey(std::string const &key) const { return m_contentBlocks.find(key) != m_contentBlocks.end(); }

    nlohmann::json getContent(std::string const &key) const
    {
        if (!hasKey(key))
        {
            throw RuntimeError("Token::getContent: token does not contain a block for key: " + key);
        }
        return m_contentBlocks.at(key);
    }

    void addContentBlock(std::string const &key, nlohmann::json blockContent)
    {
        const auto [it, success] = m_contentBlocks.insert({key, blockContent});
        if (!success)
        {
            throw RuntimeError("Token::addContentBlock: token already has a block for key: " + key);
        }
    }

    void mergeContentBlocks(Token const &token)
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

private:
    std::unordered_map<std::string, nlohmann::json> m_contentBlocks;
};

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_TOKEN_HPP_