#pragma once

#include <string>
#include <vector>

class Configure {
public:
    static Configure& instance() {
        static Configure instance;
        return instance;
    }
    void load(const std::string& path);
    void save(const std::string& path) const;
    std::string version() const { return m_version; }
    uint16_t port() const { return m_port; }
    int callSetupTime() const { return m_callSetupTime; }
    std::vector<std::string> devices() const { return m_devices; }

private:
    Configure() = default;
    ~Configure() = default;
    Configure(const Configure&) = delete;
    Configure& operator=(const Configure&) = delete;
    Configure(Configure&&) = delete;
    Configure& operator=(Configure&&) = delete;
    bool parse(const std::string& content);

private:
    std::string m_path{};
    uint16_t m_port{};
    int m_callSetupTime{};
    std::string m_version{};
    std::vector<std::string> m_devices{};
};
