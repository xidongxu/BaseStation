#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    enum Reason { Clean, Timeout, Reset, Manual, Error };

    explicit Session(asio::ip::tcp::socket socket, asio::io_context& context, int m_key);
    ~Session();

    void start();
    void close();
    void send(const std::unique_ptr<Message>& message);

private:
    void do_timeout();
    void do_receive();
    void do_write(const std::unique_ptr<Message>& message);
    void do_close(Reason reason);
    void do_disconnect(const asio::error_code& error);

private:
    int m_key{};
    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    Message::RawData m_buffer{};
    uint16_t m_port{};
    std::string m_address{};
    std::string m_number{};
    std::atomic<bool> m_closed{};
};

class Server {
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
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
