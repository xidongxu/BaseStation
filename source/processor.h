#pragma once

#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

class Message {
public:
    using RawData = std::array<uint8_t, 1024>;

    Message(const RawData& data);
    Message(int id, 
            std::string type,
            std::string from,
            std::vector<std::string> to,
            std::string func,
            int uuid,
            std::vector<uint8_t> content, 
            int result);
    ~Message();

    RawData raw() const { return m_data; }
    bool valid() const { return m_valid; };
    std::string version() const { return m_version; };
    int id() const { return m_id; }
    std::string type() { return m_type; }
    uint32_t timestamp() const { return m_timestamp; }
    std::string from() { return m_from; }
    std::vector<std::string>& to() { return m_to; }
    std::string func() { return m_func; }
    int uuid() { return m_uuid; }
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
    int m_uuid{};
    std::vector<uint8_t> m_content{};
    int m_result{};
    RawData m_data{};
};

class MessageProcessor {
public:
    enum Type { Recv, Send };
    MessageProcessor(const MessageProcessor&) = delete;
    MessageProcessor& operator=(const MessageProcessor&) = delete;
    MessageProcessor(MessageProcessor&&) = delete;
    MessageProcessor& operator=(MessageProcessor&&) = delete;
    static MessageProcessor& instance() {
        static MessageProcessor instance{};
        return instance;
    }
    void process();
    void resolve(std::unique_ptr<Message>& message);
    void append(Type type, std::unique_ptr<Message> &message);
    std::unique_ptr<Message> fetch(Type type);
    void cleanup(Type type);
    std::size_t size(Type type);
    void quit() { m_quit = true; }

private:
    MessageProcessor();
    ~MessageProcessor();

private:
    std::mutex m_mutex{};
    std::thread m_thread{};
    std::atomic<bool> m_quit{};
    std::condition_variable m_condition{};
    std::deque<std::unique_ptr<Message>> m_recv{};
    std::deque<std::unique_ptr<Message>> m_send{};
};
