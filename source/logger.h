#pragma once

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

class Logger;
class LogStream {
public:
    enum Level { Info = 0, Waring = 1, Error = 2 };

    LogStream(Logger& logger, Level level, const std::string& tag);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& msg) { m_buffer << " " << msg; return *this; }
    typedef std::ostream& (*EndlType)(std::ostream&);
    LogStream& operator<<(EndlType manip);
    template<size_t N>
    LogStream& operator<<(const std::array<uint8_t, N>& arr) { return hex(arr.data(), N); }
    LogStream& hex(const uint8_t* data, size_t len);

private:
    Level m_level{};
    Logger& m_logger;
    std::string m_tag{};
    std::stringstream m_buffer{};
};

class Logger {
public:
    using Level = LogStream::Level;

    static Logger& instance();
    void setLevel(Level level);
    LogStream log(Level level, const std::string& tag);
    void flush(Level level, const std::string& msg, const std::string& tag);

private:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(const std::string& filename);
    ~Logger();
    std::string getThread();
    std::string getLevel(Level level);
    std::string getTime();

private:
    Level m_level{};
    std::mutex m_mutex{};
    std::ofstream m_file{};
};

#define LogInfo()  Logger::instance().log(Logger::Level::Info, LOG_TAG)
#define LogWarn()  Logger::instance().log(Logger::Level::Waring, LOG_TAG)
#define LogError() Logger::instance().log(Logger::Level::Error, LOG_TAG)
