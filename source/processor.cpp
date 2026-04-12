#include <chrono>
#include <thread>
#include "cJSON.h"
#include "server.h"
#include "processor.h"
#include <iostream>

using namespace asio;
using namespace std;

void MessageBuffer::append(const Data& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto msg = std::make_unique<Message>();
    msg->data = data;
    m_queue.push_back(std::move(msg));
}

std::unique_ptr<MessageBuffer::Message> MessageBuffer::fetch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return nullptr;
    }
    auto msg = std::move(m_queue.front());
    m_queue.pop_front();
    return msg;
}

void MessageBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop_front();
    }
}

size_t MessageBuffer::size() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

MessageProcessor::MessageProcessor() {
    m_thread = std::thread(&MessageProcessor::process, this);
}

MessageProcessor::~MessageProcessor() {
    quit();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void MessageProcessor::process() {
    while (!m_quit) {
        auto message = fetch();
        if (message) {
            for (std::size_t i = 0; i < 10; ++i) {
                std::cout << std::hex << static_cast<int>(message->data[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
    }
    cleanup();
}

void MessageProcessor::append(const MessageBuffer::Data& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.append(data);
}

std::unique_ptr<MessageBuffer::Message> MessageProcessor::fetch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_buffer.fetch();
}

void MessageProcessor::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
}
