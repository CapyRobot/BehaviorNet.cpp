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

#include <behavior_net/Common.hpp>
#include <catch2/catch_test_macros.hpp>

#include <behavior_net/Token.hpp>

#include "TestsCommon.hpp"

using namespace capybot::bnet;

TEST_CASE("We can manage token content", "[PetriNet/Token]")
{
    // add and retrieve content
    {
        auto token = Token::makeUnique();

        nlohmann::json content1;
        content1["k"] = "content1";
        nlohmann::json content2;
        content2["k"] = "content2";
        token->addContentBlock("content1", content1);
        token->addContentBlock("content2", content2);

        REQUIRE(token->hasKey("content1"));
        REQUIRE(token->hasKey("content2"));
        REQUIRE_FALSE(token->hasKey("content3"));

        content1 = token->getContent("content1");
        content2 = token->getContent("content2");
        REQUIRE_BNET_THROW_AS(token->getContent("content3"), ExceptionType::RUNTIME_ERROR);

        REQUIRE(content1["k"] == "content1");
        REQUIRE(content2["k"] == "content2");
    }

    // we can merge content from another token
    {
        auto token1 = Token::makeShared();
        auto token2 = Token::makeShared();
        nlohmann::json content1;
        nlohmann::json content2;
        token1->addContentBlock("content1", content1);
        token2->addContentBlock("content2", content2);

        token1->mergeContentBlocks(token2);
        content1 = token1->getContent("content1");
        content2 = token1->getContent("content2");

        // tokens cannot have conflicting keys
        REQUIRE_BNET_THROW_AS(token1->mergeContentBlocks(token2), ExceptionType::RUNTIME_ERROR);
    }

    // throw on nullptr param
    {
        auto token = Token::makeUnique();
        Token::SharedPtr tokenNull;
        REQUIRE_BNET_THROW_AS(token->mergeContentBlocks(tokenNull), ExceptionType::RUNTIME_ERROR);
    }
}
