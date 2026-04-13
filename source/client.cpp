#include <chrono>
#include <thread>
#include "asio.hpp"
#include "cJSON.h"
#include "client.h"
#include "processor.h"
#include "logger.h"

using namespace std;
using asio::ip::tcp;

client::client(tcp::socket socket) : m_socket(std::move(socket)), m_timer(asio::system_executor()) {
    try {
        m_address = m_socket.remote_endpoint().address().to_string();
        m_port = m_socket.remote_endpoint().port();
        m_connected = true;
    }
    catch (const std::exception& error) {
        m_address = "unkonwn";
        m_port = 0;
    }
}

client::~client() {
    close(Reason::Manual);
}

void client::start() {
    do_timeout();
    do_read();
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
    do_send(message);
}

void client::do_timeout() {
    m_timer.expires_after(3s);
    m_timer.async_wait([this](const asio::error_code& error) {
            if (!error) {
                LogInfo() << "client timer has timeout";
                close(Reason::Manual);
            }
        }
    );
}

void client::do_read() {
    auto self(shared_from_this());
    asio::async_read(
        m_socket,
        asio::buffer(m_buffer, m_buffer.size()),
        asio::transfer_exactly(10),
        [this, self](const asio::error_code& error, std::size_t bytes) {
            if (!error) {
                MessageProcessor::instance().append(m_buffer);
                do_timeout();
                do_read();
            } else {
                do_disconnect(error);
            }
        }
    );
}

void client::do_send(const std::string& message) {

}

void client::do_disconnect(const asio::error_code& error) {
    Reason reason = Reason::Clean;
    switch (error.value()) {
    case asio::error::eof:
        reason = Reason::Clean;
        break;
    case asio::error::connection_reset:
    case asio::error::connection_aborted:
        reason = Reason::Reset;
        break;
    case asio::error::operation_aborted:
        reason = Reason::Manual;
        break;
    default:
        reason = Reason::Error;
        LogInfo() << "socket disconnect error: " << error.message();
        break;
    }
    close(reason);
}
