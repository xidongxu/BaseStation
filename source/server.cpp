#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "server.h"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

void server::start(uint16_t port) {
    m_acceptor = std::make_unique<tcp::acceptor>(m_context, tcp::endpoint(tcp::v4(), port));
    accept();
    m_thread = std::thread([this, port]() {
        LogInfo() << "Server start, listen port:" << port;
        m_context.run();
    });
}

void server::close() {
    asio::error_code error{};
    m_context.stop();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_acceptor && m_acceptor->is_open()) {
        m_acceptor->cancel();
        m_acceptor->close(error);
    }
    m_acceptor.reset();
}

void server::accept() {
    m_acceptor->async_accept(
        [this](asio::error_code error, tcp::socket socket) {
            if (!error) {
                auto session = std::make_shared<client>(std::move(socket), m_context);
                session->start();
            } else {
                LogError() << "Accept error: " << error.message();
            }
            accept();
        });
}

void server::send(const std::unique_ptr<Message>& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& number : message->to()) {
        if (m_sessions.contains(number)) {
            m_sessions[number]->send(message);
            return;
        }
    }
}

void server::append(std::string number, std::shared_ptr<client> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_sessions.contains(number))
        return;
    m_sessions.insert({ number, session });
    LogInfo() << "client:" << number << "login";
}

void server::remove(std::string number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_sessions.contains(number))
        return;
    if (auto it = m_sessions.find(number); it != m_sessions.end()) {
        m_sessions.erase(it);
    }
    LogInfo() << "client:" << number << "logout";
}
