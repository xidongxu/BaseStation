#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"
#include "server.h"

class Equipment : public std::enable_shared_from_this<Equipment> {
public:
    enum Login { Offline, Online, Shutdown };
    enum Voice { Idle, Calling, Ringing, Talking };

    explicit Equipment(std::string number);
    ~Equipment();
    Login login() const { return m_login; }
    Voice voice() const { return m_voice; }
    std::string number() const { return m_number; }
    void send(const std::unique_ptr<Message>& message);

private:
    Login m_login{};
    Voice m_voice{};
    std::string m_number{};
    std::shared_ptr<Session> m_session{};
};

class EquipmentManager {
public:
    EquipmentManager(const EquipmentManager&) = delete;
    EquipmentManager& operator=(const EquipmentManager&) = delete;
    EquipmentManager(EquipmentManager&&) = delete;
    EquipmentManager& operator=(EquipmentManager&&) = delete;
    static EquipmentManager& instance() {
        static EquipmentManager instance{};
        return instance;
    }
    void append(const std::string& number);
    void append(const std::vector<std::string>& numbers);
    void remove(const std::string& number);

private:
    EquipmentManager() = default;
    ~EquipmentManager() = default;

private:
    std::mutex m_mutex{};
    std::unordered_map<std::string, std::shared_ptr<Equipment>> m_equipments{};
};
