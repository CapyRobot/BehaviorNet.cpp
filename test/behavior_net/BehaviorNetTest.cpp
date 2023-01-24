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

#include <catch2/catch_test_macros.hpp>

#include "behavior_net/BehaviorNet.hpp"

using namespace capybot;

std::unique_ptr<bnet::INet> createFromSampleConfig()
{
    auto config = bnet::BehaviorNetConfig(
        "config_samples/config.json"); // TODO: create test specific config once we have a stable config format
    return bnet::create(config);
}

TEST_CASE("We can manually trigger transitions.", "[BehaviorNet]")
{
    auto net = createFromSampleConfig();

    bnet::Token::SharedPtr tokenPtr = std::make_shared<bnet::Token>();
    tokenPtr->addContentBlock("type", nlohmann::json());
    net->addToken(tokenPtr, "A");
    net->addToken(tokenPtr, "A");

    // initial marking as expected
    {
        const auto m = net->getCurrentMarking();
        REQUIRE(m.at("A") == 2);
        REQUIRE(m.at("B") == 0);
        REQUIRE(m.at("C") == 0);
        REQUIRE(m.at("D") == 0);
    }

    // marking as expected after triggering transitions
    {
        net->triggerManualTransition("T1");
        net->triggerManualTransition("T1");
        const auto m = net->getCurrentMarking();
        REQUIRE(m.at("A") == 0);
        REQUIRE(m.at("B") == 2);
        REQUIRE(m.at("C") == 2);
        REQUIRE(m.at("D") == 0);
    }

    // trigging disable transition should throw
    {
        REQUIRE_THROWS_AS(net->triggerManualTransition("T1"), bnet::LogicError);
    }
}

TEST_CASE("We can manage token content", "[BehaviorNet/Token]")
{
    // add and retrieve content
    {
        bnet::Token token;

        nlohmann::json content1;
        content1["k"] = "content1";
        nlohmann::json content2;
        content2["k"] = "content2";
        token.addContentBlock("content1", content1);
        token.addContentBlock("content2", content2);

        REQUIRE(token.hasKey("content1"));
        REQUIRE(token.hasKey("content2"));
        REQUIRE_FALSE(token.hasKey("content3"));

        content1 = token.getContent("content1");
        content2 = token.getContent("content2");
        REQUIRE_THROWS_AS(token.getContent("content3"), bnet::RuntimeError);

        REQUIRE(content1["k"] == "content1");
        REQUIRE(content2["k"] == "content2");
    }

    // we can merge content from another token
    {
        bnet::Token token1;
        bnet::Token token2;
        nlohmann::json content1;
        nlohmann::json content2;
        token1.addContentBlock("content1", content1);
        token2.addContentBlock("content2", content2);

        token1.mergeContentBlocks(token2);
        content1 = token1.getContent("content1");
        content2 = token1.getContent("content2");

        // tokens cannot have conflicting keys
        bnet::Token token3 = token2;
        REQUIRE_THROWS_AS(token2.mergeContentBlocks(token3), bnet::RuntimeError);
    }
}
