#include <chrono>
#include <memory>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "cmdcall.h"
#include "equipment.h"
#include "timer.h"

#define LOG_TAG "command"
#include "logger.h"

using namespace std;
using namespace std::chrono;

#define CALL_TARGET_EMPTY_NUMBER    1000
#define CALL_TARGET_OFFLINE         1001
#define CALL_TARGET_SHUTDOWN        1002
#define CALL_TARGET_NO_ANSWER       1003

void MakeCall::execute(std::unique_ptr<Message>& message) {
    auto id = message->id();
    auto from = message->from();
    auto to = message->to().at(0);
    auto func = message->func();
    LogInfo() << "message:" << message->func() << from << "call" << to;
    auto equipment = EquipmentManager::instance().equipment(to);
    auto result = CALL_TARGET_EMPTY_NUMBER;
    if (!equipment) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            func,
            message->content(),
            static_cast<int>(result)
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    if (equipment->state() != Equipment::Online) {
        result = equipment->state() == Equipment::Offline ? CALL_TARGET_OFFLINE : CALL_TARGET_SHUTDOWN;
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            func,
            message->content(),
            static_cast<int>(result)
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    auto timer = TimerManager::instance().create();
    TimerManager::instance().start(
        timer, 
        seconds(30), 
        [timer, id, from, func]() {
            LogInfo() << "timer:" << timer << "timeout";
            TimerManager::instance().remove(timer);
            auto result = CALL_TARGET_NO_ANSWER;
            auto response = std::make_unique<Message>(
                id,
                "RSP",
                "server",
                std::vector<std::string>{ from },
                func,
                std::vector<uint8_t>{}, 
                result
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
        static_cast<int>(timer));
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    LogInfo() << "message:" << message->func() << from << "answer call" << to;
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
    auto from = message->from();
    auto to = message->to().at(0);
    LogInfo() << "message:" << message->func() << from << "end call" << to;
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
    auto from = message->from();
    auto to = message->to().at(0);
    LogInfo() << "message:" << message->func() << from << "reject call" << to;
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
