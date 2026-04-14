#include <chrono>
#include <thread>
#include "cJSON.h"
#include "server.h"
#include "processor.h"
#include "logger.h"

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
    m_valid = true;
    cJSON_Delete(message);
}

Message::Message(int id, std::string type, 
    std::string from, 
    std::vector<std::string> to, 
    std::string func, 
    std::vector<uint8_t> content, int result) {
    auto durations = std::chrono::system_clock::now().time_since_epoch();
    auto timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(durations).count());

    m_version = "1.0";
    m_id = id;
    m_type = type;
    m_timestamp = timestamp;
    m_from = from;
    m_to = to;
    m_func = func;
    m_content = content;
    m_result = result;
    m_valid = true;

    cJSON* message = cJSON_CreateObject();
    cJSON* header = cJSON_CreateObject();
    cJSON* payload = cJSON_CreateObject();

    cJSON_AddStringToObject(header, "version", m_version.data());
    cJSON_AddNumberToObject(header, "id", m_id);
    cJSON_AddStringToObject(header, "type", m_type.data());
    cJSON_AddNumberToObject(header, "timestamp", m_timestamp);
    cJSON_AddStringToObject(header, "from", m_from.data());

    cJSON* to_array = cJSON_CreateArray();
    for (int i = 0; i < m_to.size(); i++) {
        cJSON_AddItemToArray(to_array, cJSON_CreateString(m_to.at(i).data()));
    }
    cJSON_AddItemToObject(header, "to", to_array);
    cJSON_AddItemToObject(message, "header", header);
    
    cJSON_AddStringToObject(payload, "func", m_func.data());
    cJSON* content_array = cJSON_CreateArray();
    for (int i = 0; i < m_content.size(); i++) {
        cJSON_AddItemToArray(content_array, cJSON_CreateNumber(m_content.at(i)));
    }
    cJSON_AddItemToObject(payload, "content", content_array);
    cJSON_AddNumberToObject(payload, "result", m_result);

    cJSON_AddItemToObject(message, "payload", payload);
    char* json = cJSON_Print(message);
    m_data.fill(0);
    m_data[0] = 0x55;
    m_data[1] = 0x55;
    memcpy(&m_data[2], json, strnlen(json, 1020));
    m_data[1022] = 0xFF;
    m_data[1023] = 0xFF;
    cJSON_Delete(message);
    cJSON_free(json);
}

Message::~Message() = default;

bool Message::is_heart() const {
    if (!m_valid) {
        return false;
    }
    if (m_type == "REQ" && m_func == "HEART") {
        return true;
    }
    return false;
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
    std::unique_ptr<Message> message{};
    while (!m_quit) {
        message = fetch(recv);
        if (message && message->valid()) {
            resolve(message);
        }
        message = fetch(send);
        if (message && message->valid()) {
            server::instance().send(message);
        }
    }
    cleanup(recv);
    cleanup(send);
}

void MessageProcessor::resolve(std::unique_ptr<Message>& message) {
    if (message->is_heart()) {
        LogInfo() << "Message Recv:" << message->from();
        auto response = std::make_unique<Message>(message->id(), "RESP", "0", std::vector<std::string>{ message->from() }, "HEART", std::vector<uint8_t>{}, 0);
        append(response, send);
        return;
    }
}

void MessageProcessor::append(std::unique_ptr<Message> &message, Type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (type) {
    case recv:
        m_recv.push_back(std::move(message));
        break;
    case send:
        m_send.push_back(std::move(message));
        break;
    }
}

std::unique_ptr<Message> MessageProcessor::fetch(Type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unique_ptr<Message> message = nullptr;
    switch (type) {
    case recv:
        if (!m_recv.empty()) {
            message = std::move(m_recv.front());
            m_recv.pop_front();
        }
        break;
    case send:
        if (!m_send.empty()) {
            message = std::move(m_send.front());
            m_send.pop_front();
        }
        break;
    }
    return message;
}

void MessageProcessor::cleanup(Type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (type) {
    case recv:
        while (!m_recv.empty()) {
            m_recv.pop_front();
        }
        break;
    case send:
        while (!m_send.empty()) {
            m_send.pop_front();
        }
        break;
    }
}

size_t MessageProcessor::size(Type type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t size = 0;
    switch (type) {
    case recv:
        size = m_recv.size();
        break;
    case send:
        size = m_send.size();
        break;
    }
    return size;
}
