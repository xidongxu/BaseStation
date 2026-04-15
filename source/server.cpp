#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "server.h"

#define LOG_TAG "servers"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

void Server::start(uint16_t port) {
    m_closed = false;
    localhosts(port);
    m_acceptor = std::make_unique<tcp::acceptor>(m_context, tcp::endpoint(tcp::v4(), port));
    accept();
    m_thread = std::thread([this]() {
        m_context.run();
    });
}

void Server::close() {
    asio::error_code error{};
    if (m_closed) {
        return;
    }
    m_context.stop();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_acceptor && m_acceptor->is_open()) {
        m_acceptor->cancel();
        m_acceptor->close(error);
    }
    m_acceptor.reset();
    clear();
    m_closed = true;
}

void Server::accept() {
    m_acceptor->async_accept(
        [this](asio::error_code error, tcp::socket socket) {
            if (!error) {
                auto session = std::make_shared<Session>(std::move(socket), m_context);
                session->start();
            } else {
                LogError() << "Accept error: " << error.message();
            }
            accept();
        });
}

void Server::send(const std::unique_ptr<Message>& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& number : message->to()) {
        if (m_sessions.contains(number)) {
            m_sessions[number]->send(message);
            return;
        }
    }
}

bool Server::append(std::string number, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(number);
    if (it != m_sessions.end()) {
        if (it->second != session) {
            return false;
        }
        return true;
    }
    m_sessions.insert({ number, session });
    LogInfo() << "client:" << number << "login";
    return true;
}

bool Server::remove(std::string number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_sessions.contains(number))
        return false;
    if (auto it = m_sessions.find(number); it != m_sessions.end()) {
        m_sessions.erase(it);
    }
    LogInfo() << "client:" << number << "logout";
    return true;
}

void Server::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto it = m_sessions.begin(); it != m_sessions.end()) {
        it->second->close();
        it = m_sessions.erase(it);
    }
}

void Server::localhosts(uint16_t port) {
    tcp::resolver resolver(m_context);
    auto endpoints = resolver.resolve(asio::ip::host_name(), "");
    for (auto& endpoint : endpoints) {
        if (!endpoint.endpoint().address().is_v4()) {
            continue;
        }
        auto address = endpoint.endpoint().address().to_string();
        LogInfo() << address << ":" << port;
    }
}
