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

#include <behavior_net/Common.hpp>

#include <3rd_party/nlohmann/json.hpp>

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>

#define REGISTER_NET_CONFIG_VALIDATOR(validatorFunc, validatorId)                                                      \
    static bool _registered_##validatorType = NetConfig::registerValidator(validatorFunc, validatorId);

namespace capybot
{
namespace bnet
{

class NetConfig
{
public:
    NetConfig(std::string const& configFilePath)
    {
        std::ifstream file(configFilePath);
        file >> m_config;

        validateConfig();
    }

    const nlohmann::json& get() const { return m_config; }

    /**
     * @brief Function used to validate config
     *
     * @param netConfig
     * @param errorMessages [output] list of error message in case of failure.
     * @return true on validation success
     * @return false on validation failure
     */
    using ValidatorFunc = std::function<bool(nlohmann::json const& netConfig, std::vector<std::string>& errorMessages)>;

    static bool registerValidator(ValidatorFunc validator, std::string const& validatorId)
    {
        std::cout << "[INFO] registering NetConfig validator: " << validatorId << std::endl;
        s_validators.push_back(Validator{.id = validatorId, .func = std::move(validator)});
        return true;
    }

private:
    nlohmann::json m_config;

    void validateConfig()
    {
        std::vector<std::string> errors{};
        for (auto&& validator : s_validators)
        {
            std::vector<std::string> errorMsgs{""};
            if (!validator.func(m_config, errorMsgs))
            {
                for (auto&& err : errorMsgs)
                {
                    errors.push_back("[" + validator.id + "] " + err);
                }
            }
        }

        if (!errors.empty()) // failure
        {
            std::stringstream ss;
            ss << "NetConfig::validateConfig: Failed to validate configuration. " << errors.size()
               << " errors found:\n";
            for (auto&& error : errors)
            {
                ss << "\t" << error << "\n";
            }
            throw ConfigFileError(ss.str());
        }
    }

    struct Validator
    {
        std::string id;
        ValidatorFunc func;
    };
    static std::vector<Validator> s_validators;
};

} // namespace bnet
} // namespace capybot
