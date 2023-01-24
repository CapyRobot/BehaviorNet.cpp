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
#include <sstream>
#include <string>

namespace capybot
{
namespace bnet
{

class Exception : public std::exception
{
public:
    explicit Exception(const char *message) : m_msg(message) {}
    explicit Exception(std::string const &message) : m_msg(message) {}
    virtual ~Exception() noexcept {}

    virtual const char *what() const noexcept { return m_msg.c_str(); }

protected:
    std::string m_msg;
};

class RuntimeError : public Exception
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

} // namespace bnet
} // namespace capybot

#endif // BEHAVIOR_NET_CPP_COMMON_HPP_
