#include <fstream>
#include <iostream>
#include "cJSON.h"
#include "configure.h"

#define LOG_TAG "_config"
#include "logger.h"

using namespace std;

constexpr const char* deconfigure = R"({
    "version": "1.0",
    "port": 5566,
    "callSetupTime": 30,
    "devices": [
        "16100000001",
        "16100000002"
    ]
})";

void Configure::load(const std::string& path) {
    m_path = path.empty() ? "configure.json" : path;
    std::ifstream config(m_path);
    if (config.is_open()) {
        std::stringstream buffer{};
        buffer << config.rdbuf();
        config.close();
        if (parse(buffer.str())) {
            return;
        }
        LogWarn() << "configure file parse failed, use default";
    } else {
        LogWarn() << "configure file not exist, use default";
    }
    save(deconfigure);
    parse(deconfigure);
}

void Configure::save(const std::string& configure) const {
    std::ofstream config(m_path);
    config << configure << endl;
    config.close();
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
    for (int index = 0; index < size && index < 0xFF; index++) {
        cJSON* item = cJSON_GetArrayItem(devices, index);
        if (item == NULL) {
            continue;
        }
        m_devices.push_back(std::string(item->valuestring));
    }
    cJSON_Delete(configure);
    return true;
}
