#include <chrono>
#include <thread>
#include "asio.hpp"
#include "equipment.h"

#define LOG_TAG "equipment"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

Equipment::Equipment(std::string number) : m_number(std::move(number)) {
    m_login = Login::Offline;
    m_voice = Voice::Idle;
    LogInfo() << "equipment:" << this << "create";
}

Equipment::~Equipment() {
    LogInfo() << "equipment:" << this << "destory";
}

void Equipment::send(const std::unique_ptr<Message>& message) {
    if (!m_session) {
        return;
    }
    m_session->send(message);
}

void EquipmentManager::append(const std::string& number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_equipments.contains(number)) {
        return;
    }
    m_equipments.insert({ number, std::make_shared<Equipment>(number) });
}

void EquipmentManager::append(const std::vector<std::string>& numbers) {
    for (const auto& number : numbers) {
        append(number);
    }
}

void EquipmentManager::remove(const std::string& number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_equipments.find(number);
    if (it == m_equipments.end()) {
        return;
    }
    m_equipments.erase(it);
}
