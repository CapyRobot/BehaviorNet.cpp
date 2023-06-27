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

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#define LOG(level)                                                                                                     \
    capybot::log::LogStream(capybot::log::MessageMetadata{.logLevel = capybot::log::LogLevel::level,                   \
                                                          .module = MODULE_TAG,                                        \
                                                          .fileName = __FILE__,                                        \
                                                          .lineNumber = __LINE__,                                      \
                                                          .timeMs = std::chrono::system_clock::now()})

namespace capybot
{
namespace log
{

BETTER_ENUM(LogLevel, int,
            __ALL = 0, // Setting the log level to `__ALL` will ensure all logging messages are captured.
            TRACE,     // Most detailed level, providing fine-grained information about execution.
            DEBUG,     // Detailed info for debugging purposes.
            INFO,      // General info about the program's execution; useful insights for understanding its behavior.
            WARN,      // Potential issues or concerns that don't necessarily indicate errors but require attention.
            ERROR,     // Non-critical errors that can be recovered from and may impact functionality.
            FATAL,     // Critical errors that lead to program termination.
            __OFF      // Setting the log level to `__OFF` will ensure no logging messages are captured
);

struct MessageMetadata
{
    LogLevel logLevel;
    std::string module;
    std::string fileName;
    uint32_t lineNumber;
    std::chrono::time_point<std::chrono::system_clock> timeMs;
};

class LogStream;

class Logger
{
    static constexpr char const* MODULE_TAG{"Logger"};

public:
    void setLogLevel(const LogLevel l) { m_config.logLevel = l; }
    void enableTimestamps(bool enabled = true) { m_config.timestampEnabled = enabled; }
    void enableAutoNewline(bool enabled = true) { m_config.autoNewlineEnabled = enabled; }

    // ------------------------------ static ------------------------------
    static Logger* set(std::unique_ptr<Logger> logger = nullptr);
    static Logger* get() { return set(nullptr); }

private:
    struct Config
    {
        LogLevel logLevel{LogLevel::WARN};
        bool autoNewlineEnabled{false};
        bool timestampEnabled{false};
    } m_config;

    Config const& config() const { return m_config; }

    bool shouldLog(const LogLevel logLevel) const { return logLevel >= m_config.logLevel; }
    void appendTimestamp(MessageMetadata const& meta, std::ostream& stream);

    /// @brief [lock(mutex)] check log level and call `logImpl(...)`
    void log(MessageMetadata const& meta, std::string const& msg) const;

    friend LogStream;

protected:
    virtual void logImpl(MessageMetadata const& meta, std::string const& msg) const = 0;
};

class LogStream
{
    std::stringstream m_stream;
    MessageMetadata m_metadata;

    char m_lastCharLogged{0}; // keeping track for the `autoNewlineEnabled` feature

public:
    LogStream(MessageMetadata const& metadata);
    ~LogStream();

    std::stringstream& stream() { return m_stream; };
    void flushContent();
    void appendNewLine();

    struct flush_t
    {
    };
    struct endl_t
    {
    };
};

/// @brief basic std::cout/std::cerr colored logger
class DefaultLogger : public Logger
{
public:
    void logImpl(MessageMetadata const& meta, std::string const& msg) const override final;
};

/// @brief logger with multi-sync support
class CallbackLogger : public Logger
{
public:
    using callback_t = std::function<void(MessageMetadata const& meta, std::string const& msg)>;
    void addSink(callback_t const cb) { m_sinks.push_back(cb); }
    void logImpl(MessageMetadata const& meta, std::string const& msg) const override final;

private:
    std::vector<callback_t> m_sinks;
};

constexpr LogStream::flush_t flush{};
constexpr LogStream::endl_t endl{};

template <typename T>
LogStream& operator<<(LogStream& stream, T const& value)
{
    stream.stream() << value;
    return stream;
}
template <>
inline LogStream& operator<<(LogStream& stream, LogStream::flush_t const&)
{
    stream.flushContent();
    return stream;
}

template <>
inline LogStream& operator<<(LogStream& stream, LogStream::endl_t const&)
{
    stream << "\n" << LogStream::flush_t();
    return stream;
}

template <typename T>
LogStream&& operator<<(LogStream&& stream, T const& value)
{
    stream << value;
    return std::move(stream);
}
} // namespace log
} // namespace capybot
