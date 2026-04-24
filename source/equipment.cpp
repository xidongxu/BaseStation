#include <chrono>
#include <thread>
#include "asio.hpp"
#include "equipment.h"
#include "timer.h"

#define LOG_TAG "equipment"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

Call::Call(int id, int timer, std::string caller, std::string called) : m_id(id), m_timer(timer), m_state(0), m_caller(caller), m_called(called) {}

Equipment::Equipment(std::string number) : m_number(std::move(number)) {
    m_state = State::Offline;
    m_voice = Voice::Idle;
    LogInfo() << "equipment:" << m_number << "create";
}

Equipment::~Equipment() {
    LogInfo() << "equipment:" << m_number << "destory";
}

bool Equipment::send(const std::unique_ptr<Message>& message) {
    if (!m_session) {
        LogWarn() << "equipment:" << m_number << "not login";
        return false;
    }
    m_session->send(message);
    return true;
}

bool Equipment::login(std::shared_ptr<Session>& session) {
    if (m_session) {
        LogWarn() << "equipment:" << m_number << "already login";
        return false;
    }
    LogInfo() << "equipment:" << m_number << "login";
    m_state = Equipment::Online;
    m_session = session;
    return true;
}

bool Equipment::logout(State reason) {
    if (!m_session) {
        return false;
    }
    LogInfo() << "equipment:" << m_number << "logout, reason:" << reason;
    m_state = reason;
    m_session->close();
    m_session.reset();
    clearCall();
    return true;
}

void Equipment::appendCall(std::shared_ptr<Call>& call) {
    int id = call->id();
    auto it = m_calls.find(id);
    if (it != m_calls.end()) {
        LogError() << "call:" << id << "already exits";
    }
    m_calls[id] = call;
}

void Equipment::removeCall(int id) {
    auto it = m_calls.find(id);
    if (it != m_calls.end()) {
        m_calls.erase(it);
    }
}

std::shared_ptr<Call> Equipment::findCall(int id) {
    auto it = m_calls.find(id);
    if (it == m_calls.end()) {
        LogWarn() << "call:" << id << "not found";
        return nullptr;
    }
    return it->second;
}

void Equipment::clearCall() {
    for (auto it = m_calls.begin(); it != m_calls.end();) {
        auto timer = it->second->timer();
        TimerManager::instance().remove(timer);
        it = m_calls.erase(it);
    }
}

void EquipmentManager::create(const std::vector<std::string>& numbers) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& number : numbers) {
        if (m_equipments.contains(number)) {
            LogWarn() << "equipment:" << number << "already register";
            continue;
        }
        m_equipments.insert({ number, std::make_shared<Equipment>(number) });
    }
}

std::shared_ptr<Equipment> EquipmentManager::equipment(const std::string number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return nullptr;
    }
    return it->second;
}

void EquipmentManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_equipments.begin(); it != m_equipments.end();) {
        it->second->logout(Equipment::Offline);
        it = m_equipments.erase(it);
    }
}

Equipment::State EquipmentManager::state(const std::string number) {
    if (number.empty()) {
        return Equipment::Offline;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return Equipment::Offline;
    }
    return it->second->state();
}

Equipment::Voice EquipmentManager::voice(const std::string number) {
    if (number.empty()) {
        return Equipment::Idle;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return Equipment::Idle;
    }
    return it->second->voice();
}

bool EquipmentManager::login(const std::string& number, std::shared_ptr<Session>& session) {
    if (number.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return false;
    }
    return it->second->login(session);
}

bool EquipmentManager::logout(const std::string& number, Equipment::State reason) {
    if (number.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return false;
    }
    return it->second->logout(reason);
}

bool EquipmentManager::send(const std::string& number, const std::unique_ptr<Message>& message) {
    if (number.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return false;
    }
    return it->second->send(message);
}
