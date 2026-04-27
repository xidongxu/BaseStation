#include <chrono>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "processor.h"
#include "server.h"

#define LOG_TAG "process"
#include "logger.h"

using namespace std;

Message::Message(const RawData& data) : m_valid(false), m_data(data) {
    char* start = reinterpret_cast<char*>(m_data.data()) + 2;
    if (m_data[0] != 0x55 || m_data[1] != 0x55 || start[0] == '\0') {
        LogWarn() << "Invalid message sync word";
        return;
    }
    cJSON* message = cJSON_Parse(start);
    if (message == nullptr || !cJSON_IsObject(message)) {
        LogWarn() << "Invalid message format";
        return;
    }
    cJSON* header = cJSON_GetObjectItem(message, "header");
    cJSON* payload = cJSON_GetObjectItem(message, "payload");
    if (header == nullptr || !cJSON_IsObject(header) ||
        payload == nullptr || !cJSON_IsObject(payload)) {
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
    if (version == nullptr || !cJSON_IsString(version) ||
        id == nullptr || !cJSON_IsNumber(id) ||
        type == nullptr || !cJSON_IsString(type) ||
        timestamp == nullptr || !cJSON_IsNumber(timestamp) ||
        from == nullptr || !cJSON_IsString(from) ||
        to == nullptr || !cJSON_IsArray(to)) {
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
        m_to.emplace_back(item->valuestring);
    }
    cJSON* func = cJSON_GetObjectItem(payload, "func");
    cJSON* uuid = cJSON_GetObjectItem(payload, "uuid");
    cJSON* content = cJSON_GetObjectItem(payload, "content");
    cJSON* result = cJSON_GetObjectItem(payload, "result");
    if (func == nullptr || !cJSON_IsString(func) ||
        uuid == nullptr || !cJSON_IsNumber(uuid) ||
        content == nullptr || !cJSON_IsArray(content) ||
        result == nullptr || !cJSON_IsNumber(result)) {
        cJSON_Delete(message);
        LogWarn() << "Invalid message payload";
        return;
    }
    m_func = std::string(func->valuestring);
    m_uuid = uuid->valueint;
    int size = std::min(cJSON_GetArraySize(content), static_cast<int>(m_data.size()));
    for (int index = 0; index < size; index++) {
        cJSON* item = cJSON_GetArrayItem(content, index);
        if (item == nullptr) {
            continue;
        }
        m_content.push_back(static_cast<uint8_t>(item->valueint));
    }
    m_result = result->valueint;
    m_valid = true;
    cJSON_Delete(message);
}

Message::Message(int id, std::string type,
    std::string from,
    std::vector<std::string> to,
    std::string func,
    int uuid,
    std::vector<uint8_t> content, int result) {
    auto durations = std::chrono::system_clock::now().time_since_epoch();
    auto timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(durations).count());

    m_version = "1.0";
    m_id = id;
    m_type = std::move(type);
    m_timestamp = timestamp;
    m_from = std::move(from);
    m_to = std::move(to);
    m_func = std::move(func);
    m_uuid = uuid;
    m_content = std::move(content);
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

    cJSON* toArray = cJSON_CreateArray();
    for (auto& i : m_to) {
        cJSON_AddItemToArray(toArray, cJSON_CreateString(i.data()));
    }
    cJSON_AddItemToObject(header, "to", toArray);
    cJSON_AddItemToObject(message, "header", header);

    cJSON_AddStringToObject(payload, "func", m_func.data());
    cJSON_AddNumberToObject(payload, "uuid", m_uuid);
    cJSON* contentArray = cJSON_CreateArray();
    for (unsigned char i : m_content) {
        cJSON_AddItemToArray(contentArray, cJSON_CreateNumber(i));
    }
    cJSON_AddItemToObject(payload, "content", contentArray);
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

std::string Message::details(std::size_t length) const {
    length = (length > m_data.size() - 4) ? (m_data.size() - 4) : length;
    std::string json(reinterpret_cast<const char *>(m_data.data() + 2), length);
    return json;
}

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
    registerCommands();
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
        message = fetch(Recv);
        if (message && message->valid()) {
            resolve(message);
        }
        message = fetch(Send);
        if (message && message->valid()) {
            Server::instance().send(message);
        }
    }
    cleanup(Recv);
    cleanup(Send);
}

void MessageProcessor::resolve(std::unique_ptr<Message>& message) {
    if (message->valid()) {
        auto cmd = CommandBuilder::instance().build(message->func());
        if (cmd) {
            cmd->execute(message);
            return;
        }
        LogWarn() << "Unknown command: " << message->func();
    }
}

void MessageProcessor::append(Type type, std::unique_ptr<Message> &message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (type) {
    case Recv:
        m_recv.push_back(std::move(message));
        break;
    case Send:
        m_send.push_back(std::move(message));
        break;
    }
    m_condition.notify_one();
}

std::unique_ptr<Message> MessageProcessor::fetch(Type type) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::unique_ptr<Message> message = nullptr;
    m_condition.wait(lock, [this] {
        return (!m_recv.empty() || !m_send.empty() || m_quit);
    });
    switch (type) {
    case Recv:
        if (!m_recv.empty()) {
            message = std::move(m_recv.front());
            m_recv.pop_front();
        }
        break;
    case Send:
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
    case Recv:
        while (!m_recv.empty()) {
            m_recv.pop_front();
        }
        break;
    case Send:
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
    case Recv:
        size = m_recv.size();
        break;
    case Send:
        size = m_send.size();
        break;
    }
    return size;
}
