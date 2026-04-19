#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"
#include "server.h"

class Equipment : public std::enable_shared_from_this<Equipment> {
public:
    enum State { Offline, Online, Shutdown };
    enum Voice { Idle, Calling, Ringing, Talking };

    explicit Equipment(std::string number);
    ~Equipment();
    State state() const { return m_state; }
    Voice voice() const { return m_voice; }
    std::string number() const { return m_number; }
    bool login(std::shared_ptr<Session>& session);
    bool logout();
    bool send(const std::unique_ptr<Message>& message);

private:
    State m_state{};
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
    void create(const std::vector<std::string>& numbers);
    Equipment::State state(const std::string number);
    Equipment::Voice voice(const std::string number);
    bool login(const std::string& number, std::shared_ptr<Session>& session);
    bool logout(const std::string& number);
    bool send(const std::string& number, const std::unique_ptr<Message>& message);
    void clear();

private:
    EquipmentManager() = default;
    ~EquipmentManager() = default;

private:
    std::mutex m_mutex{};
    std::unordered_map<std::string, std::shared_ptr<Equipment>> m_equipments{};
};
