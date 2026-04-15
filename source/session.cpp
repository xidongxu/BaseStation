#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "processor.h"
#include "server.h"
#include "session.h"

#define LOG_TAG "session"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

Session::Session(tcp::socket socket, asio::io_context& context) : m_socket(std::move(socket)), m_timer(context) {
    m_address = m_socket.remote_endpoint().address().to_string();
    m_port = m_socket.remote_endpoint().port();
    LogInfo() << "client:" << this << "create";
}

Session::~Session() {
    do_close(Reason::Manual);
    LogInfo() << "client:" << this << "destory";
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
    m_timer.async_wait(
        [this](const asio::error_code& error) {
            if (!error) {
                LogInfo() << "client:" << this << "timeout";
                do_close(Reason::Manual);
            }
        }
    );
}

void Session::do_receive() {
    auto self(shared_from_this());
    asio::async_read(
        m_socket,
        asio::buffer(m_buffer, m_buffer.size()),
        asio::transfer_exactly(m_buffer.size()),
        [this, self](const asio::error_code& error, std::size_t bytes) {
            if (!error) {
                bool result = true;
                auto message = std::make_unique<Message>(m_buffer);
                if (message->is_heart()) {
                    result = Server::instance().append(message->from(), self);
                    m_number = (result == true) ? message->from() : "";
                    do_timeout();
                }
                if (!result) {
                    LogWarn() << "client:" << this << "duplicate, close by server";
                    do_close(Reason::Manual);
                    return;
                }
                MessageProcessor::instance().append(MessageProcessor::Recv, message);
                do_receive();
            } else {
                do_disconnect(error);
            }
        }
    );
}

void Session::do_write(const std::unique_ptr<Message>& message) {
    auto self(shared_from_this());
    asio::async_write(
        m_socket,
        asio::buffer(message->raw()),
        [this, self](const asio::error_code& error, std::size_t bytes) {
            if (error) {
                LogInfo() << "Message send error:" << error.message();
                do_disconnect(error);
            }
        }
    );
}

void Session::do_close(Reason reason) {
    if (m_closed) {
        return;
    }
    if (m_number.empty() == false) {
        Server::instance().remove(m_number);
    }
    close();
}

void Session::do_disconnect(const asio::error_code& error) {
    Reason reason = Reason::Clean;
    switch (error.value()) {
    case asio::error::eof:
        LogInfo() << "client:" << this << "closed by client";
        reason = Reason::Clean;
        break;
    case asio::error::connection_reset:
    case asio::error::connection_aborted:
        LogInfo() << "client:" << this << "reset by client";
        reason = Reason::Reset;
        break;
    case asio::error::operation_aborted:
        LogInfo() << "client:" << this << "aborted by server";
        reason = Reason::Manual;
        break;
    default:
        LogInfo() << "client:" << this << "has error:" << error.message();
        reason = Reason::Error;
        break;
    }
    do_close(reason);
}
