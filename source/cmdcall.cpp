#include <chrono>
#include <thread>
#include "cJSON.h"
#include "cmdcall.h"
#include "timer.h"

#define LOG_TAG "command"
#include "logger.h"

using namespace std;
using namespace std::chrono;

void MakeCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
    auto timer = TimerManager::instance().create();
    TimerManager::instance().start(timer, seconds(10), [timer]() {
        LogInfo() << "timer:" << timer << "timeout";
        TimerManager::instance().remove(timer);
    });
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
}

void EndCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
}

void RejectCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
}
