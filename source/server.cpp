#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "equipment.h"
#include "server.h"

#define LOG_TAG "servers"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

Session::Session(tcp::socket socket, asio::io_context& context, int key) : m_socket(std::move(socket)), m_timer(context), m_key(key) {
    m_address = m_socket.remote_endpoint().address().to_string();
    m_port = m_socket.remote_endpoint().port();
    LogInfo() << "session:" << this << "create";
}

Session::~Session() {
    do_close(Reason::Manual);
    LogInfo() << "session:" << this << "destory";
}

void Session::start() {
    m_closed = false;
    do_timeout();
    do_receive();
}

void Session::close() {
    if (m_closed) {
        return;
    }
    asio::error_code error;
    m_timer.cancel();
    m_socket.shutdown(tcp::socket::shutdown_both, error);
    m_socket.close(error);
    m_closed = true;
}

void Session::send(const std::unique_ptr<Message>& message) {
    do_write(message);
}

void Session::do_timeout() {
    m_timer.expires_after(9s);
    auto self = weak_from_this();
    m_timer.async_wait(
        [self](const asio::error_code& error) {
            auto session = self.lock();
            if (!session) {
                return;
            }
            if (!error) {
                LogInfo() << "session:" << session.get() << "timeout";
                session->do_close(Reason::Manual);
            }
        }
    );
}

void Session::do_receive() {
    auto self = weak_from_this();
    asio::async_read(
        m_socket,
        asio::buffer(m_buffer, m_buffer.size()),
        asio::transfer_exactly(m_buffer.size()),
        [self](const asio::error_code& error, std::size_t bytes) {
            auto session = self.lock();
            if (!session) {
                return;
            }
            if (!error) {
                bool result = true;
                auto message = std::make_unique<Message>(session->m_buffer);
                if (message->is_heart()) {
                    result = Server::instance().append(session->m_key, message->from());
                    session->m_number = (result == true) ? message->from() : "";
                    session->do_timeout();
                }
                if (!result) {
                    LogWarn() << "session:" << session.get() << "close by server";
                    session->do_close(Reason::Manual);
                    return;
                }
                MessageProcessor::instance().append(MessageProcessor::Recv, message);
                session->do_receive();
            } else {
                session->do_disconnect(error);
            }
        }
    );
}

void Session::do_write(const std::unique_ptr<Message>& message) {
    auto self = weak_from_this();
    asio::async_write(
        m_socket,
        asio::buffer(message->raw()),
        [self](const asio::error_code& error, std::size_t bytes) {
            auto session = self.lock();
            if (!session) {
                return;
            }
            if (error) {
                LogInfo() << "Message send error:" << error.message();
                session->do_disconnect(error);
            }
        }
    );
}

void Session::do_close(Reason reason) {
    if (m_closed) {
        return;
    }
    Server::instance().remove(m_key);
    EquipmentManager::instance().logout(m_number);
    close();
}

void Session::do_disconnect(const asio::error_code& error) {
    Reason reason = Reason::Clean;
    switch (error.value()) {
    case asio::error::eof:
        LogInfo() << "session:" << this << "closed by session";
        reason = Reason::Clean;
        break;
    case asio::error::connection_reset:
    case asio::error::connection_aborted:
        LogInfo() << "session:" << this << "reset by session";
        reason = Reason::Reset;
        break;
    case asio::error::operation_aborted:
        LogInfo() << "session:" << this << "aborted by server";
        reason = Reason::Manual;
        break;
    default:
        LogInfo() << "session:" << this << "has error:" << error.message();
        reason = Reason::Error;
        break;
    }
    do_close(reason);
}

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
    clear();
    m_closed = true;
    LogInfo() << "server closed";
}

void Server::accept() {
    m_acceptor->async_accept(
        [this](asio::error_code error, tcp::socket socket) {
            if (!error) {
                auto session = std::make_shared<Session>(std::move(socket), m_context, m_counter);
                cache(m_counter, session);
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
        EquipmentManager::instance().send(number, message);
    }
}

bool Server::append(int key, std::string number) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto cache = m_sessions.find(key);
    if (cache == m_sessions.end()) {
        return true;
    }
    auto session = cache->second;
    m_sessions.erase(cache);
    return EquipmentManager::instance().login(number, session);
}

bool Server::remove(int key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(key);
    if (it == m_sessions.end()) {
        return false;
    }
    m_sessions.erase(it);
    return true;
}

void Server::cache(int key, std::shared_ptr<Session>& session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.insert({ key, session });
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

void Server::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        it->second->close();
        it = m_sessions.erase(it);
    }
    EquipmentManager::instance().clear();
}
