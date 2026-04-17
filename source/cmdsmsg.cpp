#include <chrono>
#include <thread>
#include "cJSON.h"
#include "cmdsmsg.h"

#define LOG_TAG "cmdsmsg"
#include "logger.h"

using namespace std;

void SendShortMsg::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
}

void RecvShortMsg::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
}
