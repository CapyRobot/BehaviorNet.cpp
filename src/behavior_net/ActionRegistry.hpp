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

#include "behavior_net/Action.hpp"
#include <map>
#include <memory>

#define REGISTER_ACTION_TYPE(actionType)                                                                               \
    static bool _registered_##actionType = ActionRegistry::registerActionType(                                         \
        [](nlohmann::json const parameters) { return std::unique_ptr<IActionImpl>(new actionType(parameters)); },      \
        #actionType);

namespace capybot
{
namespace bnet
{
class ActionRegistry
{
public:
    using ActionCreateFunction = std::function<std::unique_ptr<IActionImpl>(nlohmann::json const parameters)>;

    static bool registerActionType(ActionCreateFunction const& createFunc, std::string const& id)
    {
        const auto [_, success] = s_registry.m_createFunctionMap.insert({id, createFunc});
        return success;
    }

    static Action::UniquePtr create(ThreadPool& tp, std::string const& actionType, nlohmann::json const& parameters)
    {
        if (s_registry.m_createFunctionMap.find(actionType) == s_registry.m_createFunctionMap.end())
        {
            throw LogicError("ActionRegistry::create: requested action type has not been registered: '" + actionType +
                             "'.");
        }

        auto actionImpl = s_registry.m_createFunctionMap.at(actionType)(parameters);
        return std::make_unique<Action>(tp, actionImpl);
    }

    // static std::map<std::string, Action::UniquePtr> createActionMap(ThreadPool& tp, nlohmann::json const
    // actionsConfig)
    // {
    //     std::map<std::string, Action::UniquePtr> actionMap;
    //     for (auto&& config : actionsConfig)
    //     {
    //         actionMap.emplace(config["place_id"], create(tp, config["type"], config["params"]));
    //     }
    //     return actionMap;
    // }

private:
    static ActionRegistry s_registry;
    std::map<std::string, ActionCreateFunction> m_createFunctionMap;
};

} // namespace bnet
} // namespace capybot
