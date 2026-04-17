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
    bool append(int key, std::string number);
    bool remove(std::string number);
    bool remove(int key);

private:
    Server() = default;
    ~Server() { close(); }
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
    void storage(int key, std::shared_ptr<Session>& session);
    void cleanup();
    uint16_t listen(uint16_t start, uint16_t end);
    void localhost(uint16_t port);

private:
    int m_counter{};
    std::mutex m_mutex{};
    std::thread m_thread{};
    std::atomic<bool> m_closed{};
    asio::io_context m_context{};
    std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor{};
    std::unordered_map<int, std::shared_ptr<Session>> m_makeshifts{};
    std::unordered_map<std::string, std::shared_ptr<Session>> m_sessions{};
};
