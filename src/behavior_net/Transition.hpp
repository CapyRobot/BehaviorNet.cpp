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
#include <behavior_net/Place.hpp>
#include <behavior_net/Token.hpp>
#include <behavior_net/Types.hpp>

#include <3rd_party/nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>

namespace capybot
{
namespace bnet
{

class RegexFilter
{
public:
    RegexFilter() = delete;
    RegexFilter(std::string const& str)
        : m_filter(str)
    {
    }

    bool match(std::string const& str) const { return std::regex_match(str, m_filter); }

    std::function<bool(std::string const& str)> getFilterFunc() const
    {
        return [this](std::string const& str) { return match(str); };
    }

private:
    std::regex m_filter;
};

class Transition
{
    static constexpr const char* MODULE_TAG{"Transition"};

public:
    class Factory
    {
    public:
        static std::vector<Transition> createTransitions(nlohmann::json const& netConfig, Place::IdMap const& places)
        {
            auto transitionConfigs = netConfig.at("transitions");

            std::vector<Transition> transitions;
            for (auto&& transitionConfig : transitionConfigs)
            {
                transitions.emplace_back(transitionConfig, places);
            }

            return transitions;
        }
    };

    struct Arc
    {
        Place::SharedPtr place;
        ActionExecutionStatusSet resultStatusFilter{0U};
        std::optional<RegexFilter> contentBlockFilter;
    };

    Transition(nlohmann::json config, Place::IdMap const& places);

    std::string const& getId() const { return m_id; }

    bool isManual() const { return m_type == +TransitionType::MANUAL; }

    bool isEnabled() const
    {
        for (auto&& arc : m_inputArcs)
        {
            if (arc.place->getNumberTokensAvailable(arc.resultStatusFilter) == 0)
            {
                return false;
            }
        }
        return true;
    }

    void trigger();

private:
    std::vector<Arc> m_inputArcs;
    std::vector<Arc> m_outputArcs;
    std::string m_id;

    TransitionType m_type;
};

} // namespace bnet
} // namespace capybot
