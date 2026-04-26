#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"
#include "server.h"

class Call : public std::enable_shared_from_this<Call> {
public:
    Call(int id, uint64_t timer, std::string from, std::string to);
    ~Call() = default;
    int id() { return m_id; }
    int timer() { return m_timer; }
    int state() { return m_state; }
    void setState(int state) { m_state = state; }
    std::string from() const { return m_from; }
    std::string to() const { return m_to; }

private:
    int m_id{};
    int m_state{};
    uint64_t m_timer{};
    std::string m_from{};
    std::string m_to{};
};

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
    bool logout(State reason);
    bool send(const std::unique_ptr<Message>& message);
    void appendCall(std::shared_ptr<Call>& call);
    void removeCall(int id);
    std::shared_ptr<Call> findCall(int id);
    void clearCall();

private:
    State m_state{};
    Voice m_voice{};
    std::string m_number{};
    std::shared_ptr<Session> m_session{};
    std::unordered_map<int, std::shared_ptr<Call>> m_calls{};
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
    std::shared_ptr<Equipment> equipment(const std::string number);
    Equipment::State state(const std::string number);
    Equipment::Voice voice(const std::string number);
    bool login(const std::string& number, std::shared_ptr<Session>& session);
    bool logout(const std::string& number, Equipment::State reason);
    bool send(const std::string& number, const std::unique_ptr<Message>& message);
    void clear();

private:
    EquipmentManager() = default;
    ~EquipmentManager() = default;

private:
    std::mutex m_mutex{};
    std::unordered_map<std::string, std::shared_ptr<Equipment>> m_equipments{};
};
