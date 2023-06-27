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

#include "Logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ratio>
#include <thread>

namespace capybot
{
namespace log
{
Logger* Logger::set(std::unique_ptr<Logger> logger)
{
    static std::unique_ptr<Logger> l;
    if (logger)
    {
        if (l)
        {
            LOG(ERROR) << "Logger has already been set. Ignoring `Logger::set` call." << log::endl;
        }
        else
        {
            l = std::move(logger);
        }
    }
    if (!l) // not set yet -> create DefaultLogger
    {
        l = std::make_unique<DefaultLogger>();
        LOG(WARN) << "Using logger before setting it. Creating DefaultLogger instance." << log::endl;
    }
    return l.get();
}

void Logger::appendTimestamp(MessageMetadata const& meta, std::ostream& stream)
{
    if (m_config.timestampEnabled)
    {
        using namespace std::chrono;

        auto decimal = duration_cast<microseconds>(meta.timeMs.time_since_epoch()).count() % 1'000'000;
        auto ms = decimal / 1'000;
        auto us = decimal % 1'000;

        std::time_t tt = system_clock::to_time_t(meta.timeMs);
        std::tm tm = *std::localtime(&tt);
        stream << "[" << std::put_time(&tm, "%H:%M:%S.") << std::setfill('0') << std::setw(3) << ms << "'"
               << std::setfill('0') << std::setw(3) << us << "]";
    }
}

void Logger::log(MessageMetadata const& meta, std::string const& msg) const
{
    if (shouldLog(meta.logLevel))
    {
        static std::mutex m;
        std::unique_lock<std::mutex> lk(m);
        logImpl(meta, msg);
    }
}

void DefaultLogger::logImpl(MessageMetadata const& meta, std::string const& msg) const
{
    switch (meta.logLevel)
    {
    case LogLevel::WARN:
        std::cerr << "\x1B[33m" << msg << "\033[0m";
        break;
    case LogLevel::ERROR:
        std::cerr << "\x1B[31m" << msg << "\033[0m";
        break;
    case LogLevel::FATAL:
        std::cerr << "\x1B[1;31m" << msg << "\033[0m";
        break;
    default:
        std::cout << msg;
    }
}

void CallbackLogger::logImpl(MessageMetadata const& meta, std::string const& msg) const
{
    for (auto&& sink : m_sinks)
    {
        sink(meta, msg);
    }
}

LogStream::LogStream(MessageMetadata const& metadata)
    : m_metadata(metadata)
{
    Logger::get()->appendTimestamp(metadata, m_stream);
    m_stream << "[" << std::setfill(' ') << std::setw(5) << metadata.logLevel._to_string() << "]"
             << "[" << metadata.module << "]"
             << " ";
}

LogStream::~LogStream()
{
    flushContent();
    appendNewLine();
}

void LogStream::flushContent()
{
    if (m_stream.str().size())
    {
        Logger::get()->log(m_metadata, m_stream.str());
        m_lastCharLogged = m_stream.str().back();
        m_stream.str("");
        m_stream.clear();
    }
}

void LogStream::appendNewLine()
{
    if (Logger::get()->config().autoNewlineEnabled && m_lastCharLogged && m_lastCharLogged != '\n')
    {
        Logger::get()->log(m_metadata, "\n");
    }
}

} // namespace log
} // namespace capybot
