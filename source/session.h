#pragma once

#include <memory>
#include <mutex>
#include <string>
#include "asio.hpp"
#include "processor.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    enum Reason { Clean, Timeout, Reset, Manual, Error };

    explicit Session(asio::ip::tcp::socket socket, asio::io_context& context);
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
    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    Message::RawData m_buffer{};
    uint16_t m_port{};
    std::string m_address{};
    std::string m_number{};
    std::atomic<bool> m_closed{};
};
