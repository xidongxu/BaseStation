#pragma once

#include "asio.hpp"
#include <string>
#include <memory>
#include <mutex>
#include "processor.h"

class client : public std::enable_shared_from_this<client> {
public:
    enum Reason {
        Clean,
        Timeout,
        Reset,
        Error,
        Manual
    };

    explicit client(asio::ip::tcp::socket socket, asio::io_context& context);
    ~client();

    void start();
    void close(Reason reason);
    void send(const std::unique_ptr<Message>& message);

private:
    void do_timeout();
    void do_receive();
    void do_write(const std::unique_ptr<Message>& message);
    void do_disconnect(const asio::error_code& error);

private:
    asio::steady_timer m_timer;
    asio::ip::tcp::socket m_socket;
    std::array<uint8_t, 1024> m_buffer{};
    uint16_t m_port{};
    std::string m_address{};
    std::string m_number{};
    bool m_connected{};
};
