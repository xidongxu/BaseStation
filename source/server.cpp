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
    m_counter = 0;
    port = listen(port, port + 10);
    if (port == 0) {
        return;
    }
    accept();
    m_thread = std::thread([this]() {
        m_context.run();
    });
    localhost(port);
    LogInfo() << "server start";
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
    cleanup();
    m_closed = true;
    LogInfo() << "server closed";
}

void Server::accept() {
    m_acceptor->async_accept(
        [this](asio::error_code error, tcp::socket socket) {
            if (!error) {
                auto session = std::make_shared<Session>(std::move(socket), m_context, m_counter);
                storage(m_counter, session);
                m_counter++;
                session->start();
            } else {
                LogError() << "accept error:" << error.message();
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

bool Server::append(int key, std::string number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto cache = m_makeshifts.find(key);
    if (cache == m_makeshifts.end()) {
        return true;
    }
    auto session = cache->second;
    m_makeshifts.erase(cache);

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
    auto it = m_sessions.find(number);
    if (it == m_sessions.end()) {
        return false;
    }
    m_sessions.erase(it);
    LogInfo() << "client:" << number << "logout";
    return true;
}

bool Server::remove(int key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_makeshifts.find(key);
    if (it == m_makeshifts.end()) {
        return false;
    }
    m_makeshifts.erase(it);
    return true;
}

void Server::storage(int key, std::shared_ptr<Session>& session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_makeshifts.insert({ key, session });
}

void Server::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_makeshifts.begin(); it != m_makeshifts.end();) {
        it->second->close();
        it = m_makeshifts.erase(it);
    }
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
        it->second->close();
        it = m_sessions.erase(it);
    }
}

uint16_t Server::listen(uint16_t start, uint16_t end) {
    if (start > end || end > 65535) {
        LogError() << "port range:" << start << "-" << end << "invalid";
        return 0;
    }
    asio::ip::tcp::acceptor acceptor(m_context);
    for (uint16_t port = start; port <= end; ++port) {
        try {
            m_acceptor.reset();
            m_acceptor = std::make_unique<tcp::acceptor>(m_context, tcp::endpoint(tcp::v4(), port));
            return port;
        } catch (const std::exception& e) {
            LogError() << "port:" << port << "is not available";
            continue;
        }
    }
    return 0;
}

void Server::localhost(uint16_t port) {
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
