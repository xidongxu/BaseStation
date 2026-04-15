#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"
#include "session.h"

class Server {
public:
    static Server& instance() {
        static Server instance;
        return instance;
    }
    void start(uint16_t port);
    void close();
    void accept();
    void send(const std::unique_ptr<Message> &message);
    bool append(std::string number, std::shared_ptr<Session> session);
    bool remove(std::string number);

private:
    Server() = default;
    ~Server() { close(); }
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
    void clearup();
    void localhosts(uint16_t port);

private:
    std::mutex m_mutex{};
    std::thread m_thread{};
    std::atomic<bool> m_closed{};
    asio::io_context m_context{};
    std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor{};
    std::unordered_map<std::string, std::shared_ptr<Session>> m_sessions;
};
