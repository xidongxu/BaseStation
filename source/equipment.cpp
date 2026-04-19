#include <chrono>
#include <thread>
#include "asio.hpp"
#include "equipment.h"

#define LOG_TAG "equipment"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

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
    m_session = session;
    return true;
}

bool Equipment::logout() {
    if (!m_session) {
        return false;
    }
    LogInfo() << "equipment:" << m_number << "logout";
    m_session->close();
    m_session.reset();
    return true;
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

void EquipmentManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_equipments.begin(); it != m_equipments.end();) {
        it->second->logout();
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

bool EquipmentManager::logout(const std::string& number) {
    if (number.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        LogWarn() << "equipment:" << number << "not register";
        return false;
    }
    return it->second->logout();
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
