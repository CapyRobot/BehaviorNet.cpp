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
#include <behavior_net/Place.hpp>

#include <chrono>
#include <string_view>
#include <thread>

using namespace capybot::bnet;

inline nlohmann::json readJSONFile(std::string const& path)
{
    std::ifstream file(path);
    nlohmann::json json;
    file >> json;
    return json;
}

TEST_CASE("We can initialize places from configuration", "[PetriNet/Place]")
{
    // create functions are already covered by higher level PNet tests
    {}

    // single action per place
    {
        auto config = readJSONFile("test/petri_net/config/two_actions_one_place.json");
        auto places = Place::createPlaces(config);

        ThreadPool tp;
        REQUIRE_THROWS_AS(Place::createActions(tp, config["actions"], places), RuntimeError);
    }
}

TEST_CASE("A place can have tokens, execute action on them, and get those consumed", "[PetriNet/Place]")
{
    auto config = readJSONFile("test/petri_net/config/timer_place.json");
    auto places = Place::createPlaces(config);
    ThreadPool tp;
    Place::createActions(tp, config["actions"], places);

    auto place = places.begin()->second;
    REQUIRE(place->getId() == "A");
    REQUIRE_FALSE(place->isPassive());

    constexpr int NUMBER_TOKENS = 5;
    for (int i = 0; i < NUMBER_TOKENS; ++i)
    {
        auto token = Token::makeShared();
        place->insertToken(token); // we can insert tokens
    }

    REQUIRE(place->getNumberTokensBusy() == NUMBER_TOKENS);
    REQUIRE(place->getNumberTokensAvailable() == 0);
    REQUIRE(place->getNumberTokensTotal() == NUMBER_TOKENS);

    REQUIRE_THROWS_AS(place->consumeToken(), LogicError); // no tokens to consume

    // running for two epochs because of the timer implementation
    constexpr auto epochDuration = std::chrono::milliseconds(50);
    place->executeActionAsync();
    std::this_thread::sleep_for(epochDuration);
    place->checkActionResults();
    place->executeActionAsync();
    std::this_thread::sleep_for(epochDuration);
    place->checkActionResults();

    REQUIRE(place->getNumberTokensBusy() == 0);
    REQUIRE(place->getNumberTokensAvailable() == NUMBER_TOKENS);
    REQUIRE(place->getNumberTokensTotal() == NUMBER_TOKENS);

    for (int i = 0; i < NUMBER_TOKENS; ++i)
    {
        place->consumeToken(); // we can consume tokens
    }

    REQUIRE_THROWS_AS(place->consumeToken(), LogicError); // no tokens to consume
}
