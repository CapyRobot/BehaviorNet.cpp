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
#include <catch2/matchers/catch_matchers_all.hpp>

#include <memory>
#include <sstream>
#include <utils/Logger.hpp>

const std::string MODULE_TAG{"CapybotUtils/LoggerTests"};

using namespace capybot::log;

TEST_CASE("Works as expected", "[CapybotUtils/Logger]")
{
    int messageCounter = 0;
    int lineBreakCounter = 0;
    std::stringstream ss;
    {
        auto cbLogger = std::make_unique<CallbackLogger>();
        cbLogger->addSink([](MessageMetadata const&, std::string const& msg) { std::cout << msg; });
        cbLogger->addSink([&ss](MessageMetadata const&, std::string const& msg) { ss << msg; });
        cbLogger->addSink([&messageCounter](MessageMetadata const&, std::string const&) { messageCounter++; });
        cbLogger->addSink([&lineBreakCounter](MessageMetadata const&, std::string const& msg) {
            lineBreakCounter += std::count_if(msg.begin(), msg.end(), [](char c) { return c == '\n'; });
        });
        Logger::set(std::move(cbLogger));
    }

    // CallbackLogger
    {
        Logger::get()->setLogLevel(LogLevel::__ALL);
        Logger::get()->enableTimestamps(false);
        ss.str("");
        messageCounter = 0;

        LOG(WARN) << "log\n" << flush;
        LOG(ERROR) << "log" << endl;
        LOG(FATAL) << "log\n";

        constexpr char const* expected = "[ WARN][CapybotUtils/LoggerTests] log\n"
                                         "[ERROR][CapybotUtils/LoggerTests] log\n"
                                         "[FATAL][CapybotUtils/LoggerTests] log\n";

        CHECK_THAT(ss.str(), Catch::Matchers::Equals(expected));
        REQUIRE(messageCounter == 3);
    }

    // log level filter works
    {
        Logger::get()->setLogLevel(LogLevel::INFO);
        messageCounter = 0;

        LOG(TRACE) << "log" << endl;
        REQUIRE(messageCounter == 0);
        LOG(DEBUG) << "log" << endl;
        REQUIRE(messageCounter == 0);
        LOG(INFO) << "log" << endl;
        REQUIRE(messageCounter == 1);
        LOG(WARN) << "log" << endl;
        REQUIRE(messageCounter == 2);
        LOG(ERROR) << "log" << endl;
        REQUIRE(messageCounter == 3);
        LOG(FATAL) << "log" << endl;
        REQUIRE(messageCounter == 4);
    }

    // auto new line works
    {
        Logger::get()->setLogLevel(LogLevel::__ALL);
        lineBreakCounter = 0;

        LOG(ERROR) << "log";
        REQUIRE(lineBreakCounter == 0);
        LOG(ERROR) << "log" << endl;
        REQUIRE(lineBreakCounter == 1);

        Logger::get()->enableAutoNewline();
        LOG(ERROR) << "log";
        REQUIRE(lineBreakCounter == 2);
    }
}
