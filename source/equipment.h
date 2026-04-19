#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"
#include "server.h"

class Equipment : public std::enable_shared_from_this<Equipment> {
public:
    explicit Equipment(std::string number);
    ~Equipment() = default;
    void send(const std::unique_ptr<Message>& message);
    std::string number() const { return m_number; }

private:
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
        static EquipmentManager instance;
        return instance;
    }

private:
    EquipmentManager() = default;
    ~EquipmentManager() = default;

private:
    std::mutex m_mutex{};
    std::unordered_map<std::string, std::shared_ptr<Equipment>> m_equipments{};
};
