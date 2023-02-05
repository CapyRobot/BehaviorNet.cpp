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

#include "behavior_net/Controller.hpp"

#include <chrono>
#include <thread>

using namespace capybot;

TEST_CASE("The controller properly initialized from config files.", "[BehaviorController/Controller]")
{
    auto config = bnet::NetConfig(
        "config_samples/config.json"); // TODO: create test specific config once we have a stable config format
    auto net = bnet::PetriNet::create(config);
    bnet::Controller controller(config, std::move(net));

    controller.addToken({}, "A");
    controller.addToken({}, "A");
    controller.addToken({}, "A");

    controller.runDetached();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    controller.stop();
}