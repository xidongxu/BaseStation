#pragma once

#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

class Message {
public:
    Message(const std::array<uint8_t, 1024>& data);
    Message(int id, std::string type,
            std::string from,
            std::vector<std::string> to,
            std::string func,
            std::vector<uint8_t> content, int result);
    ~Message();

    std::array<uint8_t, 1024> data() const { return m_data; }
    bool valid() const { return m_valid; };
    std::string version() const { return m_version; };
    int id() const { return m_id; }
    std::string type() { return m_type; }
    uint32_t timestamp() const { return m_timestamp; }
    std::string from() { return m_from; }
    std::vector<std::string>& to() { return m_to; }
    std::string func() { return m_func; }
    std::vector<uint8_t>& content() { return m_content; }
    int result() const { return m_result; }
    bool is_heart() const;

private:
    bool m_valid{};
    std::string m_version{};
    int m_id{};
    std::string m_type{};
    uint32_t m_timestamp{};
    std::string m_from{};
    std::vector<std::string> m_to{};
    std::string m_func{};
    std::vector<uint8_t> m_content{};
    int m_result{};
    std::array<uint8_t, 1024> m_data{};
};

class MessageProcessor {
public:
    enum Type { recv, send };
    static MessageProcessor& instance() {
        static MessageProcessor instance;
        return instance;
    }
    void process();
    void resolve(std::unique_ptr<Message>& message);
    void append(std::unique_ptr<Message> &message, Type type);
    std::unique_ptr<Message> fetch(Type type);
    void cleanup(Type type);
    std::size_t size(Type type);
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
    std::atomic<bool> m_quit{};
    std::condition_variable m_condition{};
    std::deque<std::unique_ptr<Message>> m_recv{};
    std::deque<std::unique_ptr<Message>> m_send{};
};
