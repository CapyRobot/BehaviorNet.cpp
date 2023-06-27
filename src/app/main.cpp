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

#include <3rd_party/taywee/args.hpp>

#include <cstdlib>
#include <optional>
#include <stdexcept>

#include <behavior_net/Controller.hpp>
#include <utils/Logger.hpp>

using namespace capybot;

struct CmdLineArgs
{
    std::string configPath{"config_samples/config.json"};
    log::LogLevel logLevel{log::LogLevel::INFO};
};

std::optional<CmdLineArgs> parseArgs(int argc, char** argv)
{
    args::ArgumentParser parser("Behavior Net - a PetriNet-based behavior controller for robotics.",
                                "<epilog :: This goes after the options.>");
    args::HelpFlag help(parser, "help", "<help menu>", {'h', "help"});

    args::Positional<std::string> configPath(parser, "config_path", "Configuration file path.");
    args::ValueFlag<std::string> logLevel(parser, "log_level", "See capybot::log::LogLevel for options.",
                                          {"log_level"});

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Help&)
    {
        std::cout << parser;
        return std::nullopt;
    }
    catch (const args::ParseError& e)
    {
        std::cerr << "\n==>> Failed to parse command line arguments.\n"
                  << "==>> error info: " << e.what() << "\n\n"
                  << "==>> help:\n"
                  << parser;
        return std::nullopt;
    }

    CmdLineArgs cliArgs;
    if (configPath)
    {
        cliArgs.configPath = args::get(configPath);
    }
    if (logLevel)
    {
        try
        {
            cliArgs.logLevel = log::LogLevel::_from_string_nocase(args::get(logLevel).c_str());
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "\n==>> Failed to parse command line arguments.\n"
                      << "==>> Failed to cast log level from string.\n"
                      << "==>> error info: " << e.what() << "\n\n"
                      << "==>> help:\n"
                      << parser;
            return std::nullopt;
        }
    }
    return cliArgs;
}

void initLogger(log::LogLevel level)
{
    auto logger = std::make_unique<log::DefaultLogger>();
    log::Logger::set(std::move(logger));

    log::Logger::get()->setLogLevel(level);
    log::Logger::get()->enableTimestamps();
    log::Logger::get()->enableAutoNewline();
}

int main(int argc, char** argv)
{
    auto cliArgs = parseArgs(argc, argv);
    if (!cliArgs.has_value())
    {
        return EXIT_SUCCESS;
    }

    initLogger(cliArgs->logLevel);

    auto config = bnet::NetConfig(cliArgs->configPath);
    auto net = bnet::PetriNet::create(config);

    bnet::Controller controller(config, std::move(net));

    std::cout << "running ... " << std::endl;
    controller.run();

    return EXIT_SUCCESS;
}