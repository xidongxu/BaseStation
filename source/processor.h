#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <deque>

class MessageBuffer {
public:
    using Data = std::array<uint8_t, 1024>;
    struct Message {
        Data data;
    };
    MessageBuffer() = default;
    ~MessageBuffer() = default;
    void append(const Data& data);
    std::unique_ptr<Message> fetch();
    void clear();
    size_t size();

private:
    std::mutex m_mutex{};
    std::deque<std::unique_ptr<Message>> m_queue{};
};

class MessageProcessor {
public:
    static MessageProcessor& instance() {
        static MessageProcessor instance;
        return instance;
    }
    void process();
    void append(const MessageBuffer::Data& data);
    std::unique_ptr<MessageBuffer::Message> fetch();
    void cleanup();
    void quit() { m_quit = true; }

private:
    MessageProcessor();
    ~MessageProcessor();
    MessageProcessor(const MessageProcessor&) = delete;
    MessageProcessor& operator=(const MessageProcessor&) = delete;
    MessageProcessor(MessageProcessor&&) = delete;
    MessageProcessor& operator=(MessageProcessor&&) = delete;

private:
    std::mutex m_mutex{};
    std::thread m_thread{};
    MessageBuffer m_buffer{};
    std::atomic<bool> m_quit{};
    std::condition_variable m_condition{};
};
