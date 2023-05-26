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

#include <behavior_net/Common.hpp>
#include <behavior_net/Config.hpp>
#include <behavior_net/PetriNet.hpp>

#include "TestsCommon.hpp"

using namespace capybot::bnet;

std::unique_ptr<PetriNet> createFromSampleConfig()
{
    auto config = NetConfig(
        "config_samples/config.json"); // TODO: create test specific config once we have a stable config format
    return PetriNet::create(config);
}

TEST_CASE("We can manually trigger transitions.", "[PetriNet]")
{
    auto net = createFromSampleConfig();

    auto tokenA = Token::makeUnique();
    auto tokenB = Token::makeUnique();
    tokenA->addContentBlock("type", {});
    tokenB->addContentBlock("type", {});
    net->addToken(tokenA, "A");
    net->addToken(tokenB, "A");

    // initial marking as expected
    {
        const auto m = net->getMarking();
        REQUIRE(m["marking"]["A"] == 2);
        REQUIRE(m["marking"]["B"] == 0);
        REQUIRE(m["marking"]["C"] == 0);
        REQUIRE(m["marking"]["D"] == 0);
    }

    // marking as expected after triggering transitions
    {
        net->triggerTransition("T1");
        net->triggerTransition("T1");
        const auto m = net->getMarking();
        REQUIRE(m["marking"]["A"] == 0);
        REQUIRE(m["marking"]["B"] == 2);
        REQUIRE(m["marking"]["C"] == 2);
        REQUIRE(m["marking"]["D"] == 0);
    }

    // trigging disable transition should throw
    {
        REQUIRE_BNET_THROW_AS(net->triggerTransition("T1"), ExceptionType::LOGIC_ERROR);
    }
}

TEST_CASE("NeConfig validators work as expected.", "[PetriNet/NeConfig]")
{
    // good config
    std::ignore = NetConfig("config_samples/config.json");

    // Place
    {
        REQUIRE_BNET_THROW_AS(NetConfig("test/petri_net/config/place_duplicated_ids.json"),
                              ExceptionType::INVALID_CONFIG_FILE);
    }

    // Transition
    {
        std::ignore = NetConfig("test/petri_net/config/transition_valid.json"); // valid config
        REQUIRE_BNET_THROW_AS(NetConfig("test/petri_net/config/transition_invalid_arc.json"),
                              ExceptionType::INVALID_CONFIG_FILE);
        REQUIRE_BNET_THROW_AS(NetConfig("test/petri_net/config/transition_invalid_place.json"),
                              ExceptionType::INVALID_CONFIG_FILE);
        REQUIRE_BNET_THROW_AS(NetConfig("test/petri_net/config/transition_duplicated_ids.json"),
                              ExceptionType::INVALID_CONFIG_FILE);
    }
}
