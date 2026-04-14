#pragma once

#include "asio.hpp"
#include "client.h"
#include <memory>
#include <mutex>
#include <string>
#include "processor.h"

class server {
public:
    static server& instance() {
        static server instance;
        return instance;
    }
    void start(uint16_t port);
    void close();
    void accept();
    void send(const std::unique_ptr<Message> &message);
    void append(std::string number, std::shared_ptr<client> session);
    void remove(std::string number);

private:
    server() = default;
    ~server() { close(); }
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server(server&&) = delete;
    server& operator=(server&&) = delete;

private:
    std::mutex m_mutex{};
    std::thread m_thread{};
    asio::io_context m_context{};
    std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor{};
    std::unordered_map<std::string, std::shared_ptr<client>> m_sessions;
};
