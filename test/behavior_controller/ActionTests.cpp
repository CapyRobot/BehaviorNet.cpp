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

#include <behavior_net/Action.hpp>

#include <chrono>
#include <thread>

using namespace capybot;

// TEST_CASE("Action execution works as expected.", "[BehaviorController/Action]")
// {
//     bnet::ThreadPool tp(8);
//     nlohmann::json actionConfig = nlohmann::json::parse("{\"duration_ms\": 100}");
//     auto action = bnet::Action::Factory::create(tp, bnet::Action::Factory::TimerAction, actionConfig);

//     constexpr uint32_t NUMBER_TOKENS{4};
//     bnet::Token::SharedPtrVec tokenSrcVector(NUMBER_TOKENS);
//     for (auto&& tPtr : tokenSrcVector)
//     {
//         tPtr = std::make_shared<bnet::Token>();
//     }

//     // do not respawn action for token already in exec
//     action->executeAsync(tokenSrcVector);
//     REQUIRE(action->getEpochResults().empty()); // no results, nothing ready
//     action->executeAsync(tokenSrcVector);
//     std::this_thread::sleep_for(std::chrono::milliseconds(200));
//     REQUIRE(action->getEpochResults().size() == NUMBER_TOKENS); // all should be done

//     // call order must be respected
//     action->executeAsync(tokenSrcVector);
//     REQUIRE_THROWS_AS(action->executeAsync(tokenSrcVector), bnet::LogicError);
// }

// TEST_CASE("SleepAction works as expected.", "[BehaviorController/Action]")
// {
//     bnet::ThreadPool tp(8);
//     nlohmann::json actionConfig = nlohmann::json::parse("{\"duration_ms\": 500}");
//     auto action = bnet::Action::Factory::create(tp, bnet::Action::Factory::TimerAction, actionConfig);

//     constexpr uint32_t NUMBER_TOKENS{4};
//     bnet::Token::SharedPtrVec tokenSrcVector(NUMBER_TOKENS);
//     for (auto&& tPtr : tokenSrcVector)
//     {
//         tPtr = std::make_shared<bnet::Token>();
//     }

//     action->executeAsync(tokenSrcVector);

//     // no executions should be done before sleep time
//     {
//         const auto results = action->getEpochResults();
//         REQUIRE(results.empty());
//     }

//     // all executions should be done after sleep time
//     {
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         const auto results = action->getEpochResults();
//         REQUIRE(results.size() == NUMBER_TOKENS);
//     }
// }
