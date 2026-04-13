#include <chrono>
#include <thread>
#include "cJSON.h"
#include "server.h"
#include "processor.h"
#include "logger.h"

using namespace asio;
using namespace std;

Message::Message(const std::array<uint8_t, 1024>& data) : m_valid(false), m_data(data) {
    char* start = (char*)m_data.data() + 2;
    if (m_data[0] != 0x55 || m_data[1] != 0x55 || start[0] == '\0') {
        LogWarn() << "Invalid message sync word";
        return;
    }
    cJSON* message = cJSON_Parse(start);
    if (message == NULL || !cJSON_IsObject(message)) {
        LogWarn() << "Invalid message format";
        return;
    }
    cJSON* header = cJSON_GetObjectItem(message, "header");
    cJSON* payload = cJSON_GetObjectItem(message, "payload");
    if (header == NULL || !cJSON_IsObject(header) ||
        payload == NULL || !cJSON_IsObject(payload)) {
        cJSON_Delete(message);
        LogWarn() << "Invalid message structure";
        return;
    }
    cJSON* version = cJSON_GetObjectItem(header, "version");
    cJSON* id = cJSON_GetObjectItem(header, "id");
    cJSON* type = cJSON_GetObjectItem(header, "type");
    cJSON* timestamp = cJSON_GetObjectItem(header, "timestamp");
    cJSON* from = cJSON_GetObjectItem(header, "from");
    cJSON* to = cJSON_GetObjectItem(header, "to");
    if (version == NULL || !cJSON_IsString(version) ||
        id == NULL || !cJSON_IsNumber(id) ||
        type == NULL || !cJSON_IsString(type) ||
        timestamp == NULL || !cJSON_IsNumber(timestamp) ||
        from == NULL || !cJSON_IsString(from) ||
        to == NULL || !cJSON_IsArray(to)) {
        cJSON_Delete(message);
        LogWarn() << "Invalid message header";
        return;
    }
    m_version = std::string(version->valuestring);
    m_id = id->valueint;
    m_type = std::string(type->valuestring);
    m_timestamp = timestamp->valueint;
    m_from = std::string(from->valuestring);
    for (int i = 0; i < cJSON_GetArraySize(to); i++) {
        cJSON* item = cJSON_GetArrayItem(to, i);
        m_to.push_back(std::string(item->valuestring));
    }
    cJSON* func = cJSON_GetObjectItem(payload, "func");
    cJSON* content = cJSON_GetObjectItem(payload, "content");
    cJSON* result = cJSON_GetObjectItem(payload, "result");
    if (func == NULL || !cJSON_IsString(func) ||
        content == NULL || !cJSON_IsArray(content) ||
        result == NULL || !cJSON_IsNumber(result)) {
        cJSON_Delete(message);
        LogWarn() << "Invalid message payload";
        return;
    }
    m_func = std::string(func->valuestring);
    int size = std::min(cJSON_GetArraySize(content), int(m_data.size()));
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(content, i);
        m_content.push_back(static_cast<uint8_t>(item->valueint));
    }
    cJSON_Delete(message);
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
    LogInfo() << "Processor start";
    while (!m_quit) {
        auto message = fetch();
        if (message) {
            LogInfo() << "Message from:" << message->from();
        }
    }
    cleanup();
}

void MessageProcessor::append(std::unique_ptr<Message> &message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::move(message));
}

std::unique_ptr<Message> MessageProcessor::fetch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
        return nullptr;
    }
    auto message = std::move(m_queue.front());
    m_queue.pop_front();
    return message;
}

void MessageProcessor::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop_front();
    }
}

size_t MessageProcessor::size() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}
