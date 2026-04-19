#include <chrono>
#include <thread>
#include "asio.hpp"
#include "equipment.h"

#define LOG_TAG "equipment"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

Equipment::Equipment(std::string number) : m_number(std::move(number)) {
    
}

void Equipment::send(const std::unique_ptr<Message>& message) {
    if (!m_session) {
        return;
    }
    m_session->send(message);
}
