#include <fstream>
#include <iostream>
#include "cJSON.h"
#include "configure.h"

#define LOG_TAG "_config"
#include "logger.h"

using namespace std;

bool Configure::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LogError() << "file:" << path << "open failed";
        return false;
    }
    std::stringstream buffer{};
    buffer << file.rdbuf();
    file.close();
    m_path = path;
    cJSON* configure = cJSON_Parse(buffer.str().c_str());
    if (!configure) {
        LogError() << "file:" << path << "parse failed, content:" << buffer.str();
        return false;
    }
    if (configure == NULL || !cJSON_IsObject(configure)) {
        LogWarn() << "Invalid configure format";
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
        LogWarn() << "Invalid configure structure";
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
    return true;
}

void Configure::save(const std::string& path) const {
    LogInfo() << "save configure to file not implement";
}
