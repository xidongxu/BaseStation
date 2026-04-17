#include <chrono>
#include <memory>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "cmdcall.h"
#include "timer.h"

#define LOG_TAG "command"
#include "logger.h"

using namespace std;
using namespace std::chrono;

void MakeCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
    auto timer = TimerManager::instance().create();
    auto id = message->id();
    auto call = message->from();
    auto func = message->func();
    TimerManager::instance().start(
        timer, 
        seconds(30), 
        [timer, id, call, func]() {
            LogInfo() << "timer:" << timer << "timeout";
            TimerManager::instance().remove(timer);
            auto response = std::make_unique<Message>(
                id,
                "RSP",
                "server",
                std::vector<std::string>{ call },
                func,
                std::vector<uint8_t>{}, 
                1
            );
            MessageProcessor::instance().append(MessageProcessor::Send, response);
        });
    auto request = std::make_unique<Message>(
        message->id(), 
        message->type(), 
        message->from(), 
        message->to(),
        message->func(),
        message->content(), 
        static_cast<int>(timer)
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
    auto timer = message->result();
    TimerManager::instance().remove(timer);
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        message->func(),
        message->content(),
        0
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void EndCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
    auto timer = message->result();
    TimerManager::instance().remove(timer);
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        message->func(),
        message->content(),
        0
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RejectCall::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "message:" << message->func();
    auto timer = message->result();
    TimerManager::instance().remove(timer);
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        message->func(),
        message->content(),
        0
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}
