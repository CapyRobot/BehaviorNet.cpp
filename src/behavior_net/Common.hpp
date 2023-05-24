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

#include <3rd_party/better_enums/enums.h>
#include <3rd_party/nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>

#include <behavior_net/Types.hpp>

#define THROW_ON_NULLPTR(var, moduleNameStr)                                                                           \
    {                                                                                                                  \
        if (var == nullptr)                                                                                            \
        {                                                                                                              \
            throw Exception(ExceptionType::RUNTIME_ERROR, std::string(moduleNameStr) + ": THROW_ON_NULLPTR.")          \
                .appendMetadata("variable name", #var);                                                                \
        }                                                                                                              \
    }

namespace capybot
{
namespace bnet
{

class Exception final : public std::exception
{
public:
    explicit Exception(ExceptionType type, const char* message)
        : m_msg(message)
        , m_type(type)
    {
    }
    explicit Exception(ExceptionType type, std::string const& message)
        : Exception(type, message.c_str())
    {
    }
    ~Exception() noexcept {}

    const char* what() const noexcept
    {
        computeFullErrorMessage();
        return m_fullErrorMsg.c_str();
    }

    template <typename ValueT>
    Exception& appendMetadata(std::string const& key, ValueT value)
    {
        if (!m_metadata.has_value())
        {
            m_metadata = nlohmann::json{};
        }

        m_metadata.value()[key] = value;

        return *this;
    }

    ExceptionType const& type() const { return m_type; }

protected:
    void computeFullErrorMessage() const
    {
        std::stringstream ss;
        ss << "[type = " << m_type._to_string() << "] " << m_msg << "\n";

        if (m_metadata.has_value())
        {
            ss << "\nException metadata:\n" << m_metadata.value().dump(4) << "\n\n";
        }
        m_fullErrorMsg = ss.str();
    }

    std::string m_msg;
    ExceptionType m_type;
    std::optional<nlohmann::json> m_metadata;

    mutable std::string m_fullErrorMsg;
};

namespace log
{
inline void timePoint(std::string const& msg)
{
    static std::mutex m;
    std::unique_lock<std::mutex> lk(m);

    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tpMs = tp.time_since_epoch();
    std::cout << "[" << tpMs.count() % 10'000 << " ms] " << msg << std::endl;
}
} // namespace log

} // namespace bnet
} // namespace capybot
