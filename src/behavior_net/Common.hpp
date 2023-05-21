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

#ifndef BEHAVIOR_NET_CPP_COMMON_HPP_
#define BEHAVIOR_NET_CPP_COMMON_HPP_

#include <exception>
#include <mutex>
#include <sstream>
#include <string>

namespace capybot
{
namespace bnet
{

class Exception : public std::exception
{
public:
    explicit Exception(const char* message)
        : m_msg(message)
    {
    }
    explicit Exception(std::string const& message)
        : m_msg(message)
    {
    }
    virtual ~Exception() noexcept {}

    virtual const char* what() const noexcept { return m_msg.c_str(); }

    // TODO: add support for exception metadata

protected:
    std::string m_msg;
};

class RuntimeError : public Exception // TODO: error codes better?
{
public:
    using Exception::Exception;
};

class LogicError : public Exception
{
public:
    using Exception::Exception;
};

class InvalidValueError : public Exception
{
public:
    using Exception::Exception;
};

class NotImplementedError : public Exception
{
public:
    using Exception::Exception;
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

#endif // BEHAVIOR_NET_CPP_COMMON_HPP_
