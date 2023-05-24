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

#include <3rd_party/nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <unordered_map>

#include <behavior_net/Common.hpp>

namespace capybot
{
namespace bnet
{

class Token
{
public:
    static constexpr uint64_t INVALID_TOKEN_ID{0UL};

    using UniquePtr = std::unique_ptr<Token>;
    using SharedPtr = std::shared_ptr<Token>;
    using ConstSharedPtr = std::shared_ptr<const Token>;

    Token() = default;
    ~Token() = default;

    inline static SharedPtr makeShared() { return std::make_shared<Token>(); }
    inline static UniquePtr makeUnique() { return std::make_unique<Token>(); }

    bool hasKey(std::string const& key) const { return m_contentBlocks.find(key) != m_contentBlocks.end(); }

    nlohmann::json getContent(std::string const& key) const
    {
        if (!hasKey(key))
        {
            throw Exception(ExceptionType::RUNTIME_ERROR, "Token::getContent: token does not contain a block for key.")
                .appendMetadata("key", key);
        }
        return m_contentBlocks.at(key);
    }

    void addContentBlock(std::string const& key, nlohmann::json blockContent)
    {
        const auto [it, success] = m_contentBlocks.insert({key, blockContent});
        if (!success)
        {
            throw Exception(ExceptionType::RUNTIME_ERROR, "Token::addContentBlock: token already has a block for key.")
                .appendMetadata("key", key);
        }
    }

    void mergeContentBlocks(Token::SharedPtr& token)
    {
        THROW_ON_NULLPTR(token, "Token::mergeContentBlocks");

        auto mergedBlocks = m_contentBlocks;
        for (auto block : token->m_contentBlocks)
        {
            const auto [it, success] = mergedBlocks.insert(block);
            if (!success)
            {
                throw Exception(ExceptionType::RUNTIME_ERROR,
                                "Token::mergeContentBlocks: token already has a block for key.")
                    .appendMetadata("key", block.first);
            }
        }
        m_contentBlocks.swap(mergedBlocks);
    }

    void filterContentBlocks(std::function<bool(std::string const& key)> filter)
    {
        decltype(m_contentBlocks) matchedBlocks;
        for (auto it = m_contentBlocks.begin(); it != m_contentBlocks.end(); ++it)
        {
            if (filter(it->first))
            {
                matchedBlocks.insert(*it);
            }
        }
        m_contentBlocks.swap(matchedBlocks);
    }

private:
    std::unordered_map<std::string, nlohmann::json> m_contentBlocks;
};

} // namespace bnet
} // namespace capybot
