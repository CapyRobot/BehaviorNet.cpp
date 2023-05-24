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

#include <algorithm>
#include <string>

#include <behavior_net/Config.hpp>
#include <behavior_net/Transition.hpp>
#include <behavior_net/Types.hpp>

namespace capybot
{
namespace bnet
{

void validateArcConfig(nlohmann::json const& arcConfig, std::vector<std::string>& errorMessages,
                       std::optional<nlohmann::json const> placeConfigsOpt)
{
    // place id exists
    const auto placeIdOpt = getValueAtKey<std::string>(arcConfig, "place_id", errorMessages);
    if (placeIdOpt.has_value() && placeConfigsOpt.has_value())
    {
        const auto pred = [&placeIdOpt](nlohmann::json const& place) {
            return place.at("place_id").get<std::string>() == placeIdOpt.value();
        };
        if (std::find_if(std::begin(placeConfigsOpt.value()), std::end(placeConfigsOpt.value()), pred) ==
            std::end(placeConfigsOpt.value()))
        {
            errorMessages.push_back("Arc place_id `" + placeIdOpt.value() + "` not found in `places`.");
        }
    }

    // has type
    const auto typeStrOpt = getValueAtKey<std::string>(arcConfig, "type", errorMessages);
    ArcType type{ArcType::UNDEFINED};
    if (typeStrOpt.has_value())
    {
        const auto typeOpt = ArcType::_from_string_nocase_nothrow(typeStrOpt.value().c_str());
        if (!typeOpt)
        {
            errorMessages.push_back("Invalid arc type `" + typeStrOpt.value() + "`.");
        }
        else
        {
            type = typeOpt.value();
        }
    }

    // action_result_filter is only allowed for input arcs
    if (arcConfig.contains("action_result_filter"))
    {
        // is valid
        if (arcConfig.at("action_result_filter").is_array())
        {
            for (auto&& filterStr : arcConfig.at("action_result_filter"))
            {
                const auto filterOpt = getValue<std::string>(filterStr, errorMessages);
                if (filterOpt.has_value() &&
                    !ActionExecutionStatus::_from_string_nocase_nothrow(filterOpt.value().c_str()))
                {
                    errorMessages.push_back("Cannot convert `action_result_filter` value to ActionExecutionStatus: " +
                                            filterOpt.value());
                }
            }
        }
        else
        {
            errorMessages.push_back("`action_result_filter` is expected to be an array.");
        }

        if (type != +ArcType::INPUT)
        {
            errorMessages.push_back("`action_result_filter` is only allowed for input arcs.");
        }
    }

    // token_content_filter is only allowed for output arcs
    if (arcConfig.contains("token_content_filter"))
    {
        // is valid
        std::ignore = getValueAtKey<std::string>(arcConfig, "token_content_filter", errorMessages);

        if (type != +ArcType::OUTPUT)
        {
            errorMessages.push_back("`token_content_filter` is only allowed for output arcs.");
        }
    }
}

bool validateTransitionsConfig(nlohmann::json const& netConfig, std::vector<std::string>& errorMessages)
{
    errorMessages.clear();

    const auto transitionConfigsOpt =
        getValueAtPath<nlohmann::json const>(netConfig, {"petri_net", "transitions"}, errorMessages);
    if (!transitionConfigsOpt.has_value())
    {
        return false;
    }

    std::vector<std::string> ids{};
    for (auto&& transitionConfig : transitionConfigsOpt.value())
    {
        // no repeated ids
        {
            const auto id = getValueAtKey<std::string>(transitionConfig, "transition_id", errorMessages);
            if (id.has_value())
            {
                if (std::find(ids.begin(), ids.end(), id.value()) != ids.end())
                {
                    errorMessages.push_back("Repeated `transition_id`: " + id.value());
                }
                else
                {
                    ids.push_back(id.value());
                }
            }
        }

        // has valid type
        const auto typeStrOpt = getValueAtKey<std::string>(transitionConfig, "transition_type", errorMessages);
        if (typeStrOpt.has_value())
        {
            const auto typeOpt = TransitionType::_from_string_nocase_nothrow(typeStrOpt.value().c_str());
            if (!typeOpt)
            {
                errorMessages.push_back("Invalid transition type `" + typeStrOpt.value() + "`.");
            }
        }

        // arcs
        {
            const auto arcConfigsOpt =
                getValueAtKey<nlohmann::json const>(transitionConfig, "transition_arcs", errorMessages);
            if (arcConfigsOpt.has_value())
            {

                const auto placeConfigsOpt =
                    getValueAtPath<nlohmann::json const>(netConfig, {"petri_net", "places"}, errorMessages);
                for (auto&& arcConfig : arcConfigsOpt.value())
                {
                    validateArcConfig(arcConfig, errorMessages, placeConfigsOpt);
                }
            }
        }
    }

    return errorMessages.empty();
}

REGISTER_NET_CONFIG_VALIDATOR(&validateTransitionsConfig, "TransitionsConfigValidator");

Transition::Transition(nlohmann::json config, Place::IdMap const& places)
    : m_id(config.at("transition_id").get<std::string>())
    , m_type(TransitionType::UNDEFINED)
{
    /**
     * From here on, the config is assumed to be valid. See `validateTransitionsConfig`
     */

    if (config.contains("transition_type"))
    {
        m_type = TransitionType::_from_string_nocase(config.at("transition_type").get<std::string>().c_str());
    }
    else
    {
        std::cerr << "[WARN] Transition::Transition: undefined transition type, using default `AUTO`. transition_id: "
                  << m_id << std::endl;
        m_type = TransitionType::AUTO;
    }
    if (m_type == +TransitionType::UNDEFINED)
    {
        throw Exception(ExceptionType::INVALID_CONFIG_FILE, "Transition::Transition: uninitialized transition type.")
            .appendMetadata("transition_id", m_id)
            .appendMetadata("transition_type", config.at("transition_type").get<std::string>());
    }

    for (auto&& arcConfig : config.at("transition_arcs"))
    {
        Arc arc;
        const auto placeId = arcConfig.at("place_id").get<std::string>();
        arc.place = places.at(placeId);

        if (arcConfig.contains("action_result_filter"))
        {
            for (auto const& status : arcConfig.at("action_result_filter"))
            {
                arc.resultStatusFilter.set(
                    ActionExecutionStatus::_from_string_nocase(status.get<std::string>().c_str()));
            }
        }
        else
        {
            arc.resultStatusFilter = 0U;
        }

        if (arcConfig.contains("token_content_filter"))
        {
            arc.contentBlockFilter = RegexFilter(arcConfig.at("token_content_filter").get<std::string>());
        }

        const auto type = ArcType::_from_string_nocase(arcConfig.at("type").get<std::string>().c_str());
        if (type == +ArcType::OUTPUT)
        {
            m_outputArcs.push_back(arc);
        }
        else if (type == +ArcType::INPUT)
        {
            m_inputArcs.push_back(arc);
        }
        else
        {
            throw Exception(ExceptionType::INVALID_CONFIG_FILE, "Transition::Transition: invalid arc type.")
                .appendMetadata("transition_id", m_id)
                .appendMetadata("arc type", arcConfig.at("type").get<std::string>());
        }
    }
}

void Transition::trigger()
{
    if (!isEnabled())
    {
        throw Exception(ExceptionType::LOGIC_ERROR,
                        "Transition::trigger: trying to trigger disabled transition. Use `isEnabled` first.")
            .appendMetadata("transition_id", m_id);
    }

    std::vector<Token::SharedPtr> consumedTokens;
    for (auto&& arc : m_inputArcs)
    {
        consumedTokens.push_back(arc.place->consumeToken(arc.resultStatusFilter));
    }

    auto outToken = Token::makeShared();
    for (auto&& t : consumedTokens)
    {
        outToken->mergeContentBlocks(t);
    }

    for (auto&& arc : m_outputArcs)
    {
        if (arc.contentBlockFilter.has_value())
        {
            auto filteredToken = Token::makeShared();
            filteredToken->mergeContentBlocks(outToken);
            filteredToken->filterContentBlocks(arc.contentBlockFilter.value().getFilterFunc());
            arc.place->insertToken(filteredToken);
        }
        else
        {
            arc.place->insertToken(outToken);
        }
    }
}

} // namespace bnet
} // namespace capybot
