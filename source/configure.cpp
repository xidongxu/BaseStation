#include <fstream>
#include <iostream>
#include "cJSON.h"
#include "configure.h"

#define LOG_TAG "_config"
#include "logger.h"

using namespace std;

constexpr const char* default_configure = R"({
    "version": "1.0",
    "port": 5566,
    "callSetupTime": 30,
    "devices": [
        "16100000001",
        "16100000002"
    ]
})";

void Configure::load(const std::string& path) {
    m_path = path;
    std::string content{};
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream buffer{};
        buffer << file.rdbuf();
        file.close();
        content = buffer.str();
        if (parse(content)) {
            return;
        }
        LogWarn() << "configure file parse failed, use default";
    } else {
        LogWarn() << "configure file not exist, use default";
    }
    content = default_configure;
    parse(content);
}

void Configure::save(const std::string& path) const {
    LogInfo() << "save configure to file not implement";
}

bool Configure::parse(const std::string& content) {
    cJSON* configure = cJSON_Parse(content.c_str());
    if (!configure) {
        LogError() << "configure parse failed, content:" << content;
        return false;
    }
    if (configure == NULL || !cJSON_IsObject(configure)) {
        LogWarn() << "configure format invalid";
        return false;
    }
    cJSON* version = cJSON_GetObjectItem(configure, "version");
    cJSON* port = cJSON_GetObjectItem(configure, "port");
    cJSON* callSetupTime = cJSON_GetObjectItem(configure, "callSetupTime");
    cJSON* devices = cJSON_GetObjectItem(configure, "devices");
    if (version == NULL || !cJSON_IsString(version) ||
        port == NULL || !cJSON_IsNumber(port) ||
        callSetupTime == NULL || !cJSON_IsNumber(callSetupTime) ||
        devices == NULL || !cJSON_IsArray(devices)) {
        cJSON_Delete(configure);
        LogWarn() << "configure structure invalid";
        return false;
    }
    m_version = std::string(version->valuestring);
    m_port = port->valueint;
    m_callSetupTime = callSetupTime->valueint;
    int size = cJSON_GetArraySize(devices);
    for (int index = 0; index < size; index++) {
        cJSON* item = cJSON_GetArrayItem(devices, index);
        if (item == NULL) {
            continue;
        }
        m_devices.push_back(std::string(item->valuestring));
    }
    cJSON_Delete(configure);
}
