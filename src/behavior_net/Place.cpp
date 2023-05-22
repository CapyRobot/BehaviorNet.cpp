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

#include <behavior_net/Config.hpp>
#include <behavior_net/Place.hpp>

namespace capybot
{
namespace bnet
{

bool validatePlacesConfig(nlohmann::json const& netConfig, std::vector<std::string>& errorMessages)
{
    errorMessages.clear();

    if (!netConfig.contains("petri_net") || !netConfig.at("petri_net").contains("places"))
    {
        errorMessages.push_back("Net config does not contain `petri_net.places`.");
        return false;
    }
    auto placeConfigs = netConfig.at("petri_net").at("places");

    // no repeated ids
    std::vector<std::string> ids{};
    for (auto&& placeConfig : placeConfigs)
    {
        const auto id = placeConfig.at("place_id").get<std::string>();
        if (std::find(ids.begin(), ids.end(), id) != ids.end())
        {
            errorMessages.push_back("Repeated `place_id`: " + id);
        }
        else
        {
            ids.push_back(id);
        }
    }

    return errorMessages.empty();
}

REGISTER_NET_CONFIG_VALIDATOR(&validatePlacesConfig, "PlacesConfigValidator");

} // namespace bnet
} // namespace capybot
