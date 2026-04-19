#include "logger.h"

using namespace std;

LogStream::LogStream(Logger& logger, Level level, const std::string& tag) : m_logger(logger), m_level(level), m_tag(tag) {}

LogStream::~LogStream() {
    if (!m_buffer.str().empty()) {
        m_logger.flush(m_level, m_buffer.str(), m_tag);
    }
}

LogStream& LogStream::operator<<(EndlType manip) {
    if (manip == static_cast<EndlType>(&std::endl<char, std::char_traits<char>>)) {
        m_logger.flush(m_level, m_buffer.str(), m_tag);
        m_buffer.str("");
        m_buffer.clear();
    }
    return *this;
}

LogStream& LogStream::hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << " [hex] ";
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]) << " ";
    }
    oss << std::dec;
    m_buffer << oss.str();
    return *this;
}

void Logger::setLevel(Level level) {
    m_level = level;
}

Logger::Logger(const std::string& filename) : m_level(Level::Info) {
    m_file.open(filename, std::ios::app);
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void Logger::flush(Level level, const std::string& msg, const std::string& tag) {
    if (level < m_level) 
        return;
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string finalMsg =
        "[" + getTime() + "]"
        "[" + getLevel(level) + "]"
        "[" + getThread() + "]"
        + (tag.empty() ? "" : "[" + tag + "]")
        + msg;
    std::cout << finalMsg << std::endl;
    if (m_file.is_open()) {
        m_file << finalMsg << std::endl;
    }
}

std::string Logger::getThread() {
    std::ostringstream oss;
    oss << std::setw(8)
        << std::setfill('0')
        << std::this_thread::get_id();
    return oss.str();
}

std::string Logger::getLevel(Level level) {
    switch (level) {
    case Level::Info: 
        return "I";
    case Level::Waring: 
        return "W";
    case Level::Error: 
        return "E";
    }
    return "U";
}

std::string Logger::getTime() {
    time_t now = time(nullptr);
    char buf[64]{};
#ifdef _WIN32
    tm tm_buf{};
    localtime_s(&tm_buf, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
#else
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
#endif
    return buf;
}

LogStream Logger::log(Level level, const std::string &tag) {
    return LogStream(*this, level, tag);
}
