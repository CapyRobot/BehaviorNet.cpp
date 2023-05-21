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

#include <optional>
#include <string>

#include "3rd_party/nlohmann/json.hpp"

#include "behavior_net/Token.hpp"

namespace capybot
{
namespace bnet
{

/**
 * @brief configuration parameter that may contain a value or be dependent on a token content
 *
 * For example, if `configParam` is "@token{abc.def.ghi}", `get(token)` will return the value of
 * `token.getContent("abc")["def"]["ghi"]`.
 *
 * @tparam T expected parameter type
 */
template <typename T>
class ConfigParameter
{
public:
    ConfigParameter(nlohmann::json const& configParam)
        : m_value(std::nullopt)
        , m_path()
    {
        std::string str;
        try
        {
            str = configParam.get<std::string>();
        }
        catch (const nlohmann::json::exception& e)
        {
            m_value = configParam.get<T>();
            return;
        }

        if (str.find("@token") != std::string::npos)
        {
            m_path = split(getContentBetweenChars(str, '{', '}'), '.');
        }
        else
        {
            m_value = configParam.get<T>();
        }
    }

    T get(Token const& token) const
    {
        if (m_value.has_value())
        {
            return m_value.value();
        }
        else
        {
            const auto contentBlockKey = m_path.at(0);
            nlohmann::json data = token.getContent(contentBlockKey);
            for (auto it = m_path.begin() + 1; it != m_path.end(); ++it)
            {
                data = data.at(*it);
            }
            return data.get<T>();
        }
    }

private:
    std::optional<T> m_value;        // used if `configParam` is a direct value
    std::vector<std::string> m_path; // stores parameter path (e.g., "abc","def","ghi") if not a direct value

    static inline std::string getContentBetweenChars(const std::string& str, char start, char end)
    {
        size_t startPos = str.find(start);
        if (startPos == std::string::npos)
            return "";

        size_t endPos = str.find(end, startPos + 1);
        if (endPos == std::string::npos)
            return "";

        return str.substr(startPos + 1, endPos - startPos - 1);
    }

    static inline std::vector<std::string> split(const std::string& str, char delimiter)
    {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find(delimiter, start)) != std::string::npos)
        {
            tokens.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        tokens.push_back(str.substr(start));
        return tokens;
    }
};

} // namespace bnet
} // namespace capybot
