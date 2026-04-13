#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "client.h"
#include "processor.h"
#include "server.h"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

client::client(tcp::socket socket) : m_socket(std::move(socket)), m_timer(asio::system_executor()) {
    m_address = m_socket.remote_endpoint().address().to_string();
    m_port = m_socket.remote_endpoint().port();
    m_connected = true;
    LogInfo() << "client:" << this << "create";
}

client::~client() {
    close(Reason::Manual);
    LogInfo() << "client:" << this << "destory";
}

void client::start() {
    do_timeout();
    do_receive();
}

void client::close(Reason reason) {
    if (m_connected == false) {
        return;
    }
    asio::error_code error;
    m_timer.cancel();
    m_socket.shutdown(tcp::socket::shutdown_both, error);
    m_socket.close(error);
    m_connected = false;
}

void client::write(const std::string& message) {

}

void client::do_timeout() {
    m_timer.expires_after(9s);
    m_timer.async_wait(
        [this](const asio::error_code& error) {
            if (!error) {
                LogInfo() << "client:" << this << "timeout";
                close(Reason::Manual);
            }
        }
    );
}

void client::do_receive() {
    auto self(shared_from_this());
    asio::async_read(
        m_socket,
        asio::buffer(m_buffer, m_buffer.size()),
        asio::transfer_exactly(1024),
        [this, self](const asio::error_code& error, std::size_t bytes) {
            if (!error) {
                auto message = std::make_unique<Message>(m_buffer);
                if (message->is_heart()) {
                    m_number = message->from();
                    server::instance().append(message->from(), self);
                }
                MessageProcessor::instance().append(message);
                do_timeout();
                do_receive();
            } else {
                do_disconnect(error);
            }
        }
    );
}

void client::do_disconnect(const asio::error_code& error) {
    Reason reason = Reason::Clean;
    switch (error.value()) {
    case asio::error::eof:
        reason = Reason::Clean;
        LogInfo() << "client:" << this << "closed by client";
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
        reason = Reason::Error;
        LogInfo() << "client:" << this << "has error:" << error.message();
        break;
    }
    close(reason);
}
