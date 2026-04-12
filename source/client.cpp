#include <chrono>
#include <thread>
#include "cJSON.h"
#include "asio.hpp"
#include "client.h"
#include <iostream>
#include "processor.h"

using namespace std;
using asio::ip::tcp;

client::client(tcp::socket socket) : m_socket(std::move(socket)) {
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

void client::start(Disconnect disconnect) {
    m_disconnect = disconnect;
    do_read();
}

void client::close(Reason reason) {
    if (m_connected == false) {
        return;
    }
    asio::error_code error;
    m_socket.shutdown(tcp::socket::shutdown_both, error);
    m_socket.close(error);
    m_connected = false;

    if (m_disconnect) {
        m_disconnect(reason, m_number);
    }
}

void client::write(const std::string& message) {
    do_send(message);
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
                do_read();
            }
            else {
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
        std::cerr << "Unexpected error: " << error.message() << std::endl;
        break;
    }
    close(reason);
}
