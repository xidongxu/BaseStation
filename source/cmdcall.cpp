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

#define CALL_FUNC                   "CALL_NOTIFY"

#define CALL_TARGET_EMPTY_NUMBER    1000
#define CALL_TARGET_OFFLINE         1001
#define CALL_TARGET_SHUTDOWN        1002
#define CALL_TARGET_NO_ANSWER       1003

void MakeCall::execute(std::unique_ptr<Message>& message) {
    auto id = message->id();
    auto from = message->from();
    auto to = message->to().at(0);
    auto func = message->func();
    auto uuid = message->uuid();
    LogInfo() << "message:" << message->func() << from << "call" << to;
    auto equipment = EquipmentManager::instance().equipment(to);
    // Notify caller that the called phone number is empty.
    auto result = CALL_TARGET_EMPTY_NUMBER;
    if (!equipment) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            CALL_FUNC,
            uuid,
            message->content(),
            static_cast<int>(result)
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    // Notify caller that the called phone is not online.
    if (equipment->state() != Equipment::Online) {
        result = equipment->state() == Equipment::Offline ? CALL_TARGET_OFFLINE : CALL_TARGET_SHUTDOWN;
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            CALL_FUNC,
            uuid,
            message->content(),
            static_cast<int>(result)
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    // Start the answer wait timer.
    auto timer = TimerManager::instance().create();
    TimerManager::instance().start(
        timer, 
        seconds(30), 
        [timer, id, from, func, uuid]() {
            LogInfo() << "timer:" << timer << "timeout";
            TimerManager::instance().remove(timer);
            auto result = CALL_TARGET_NO_ANSWER;
            auto response = std::make_unique<Message>(
                id,
                "RSP",
                "server",
                std::vector<std::string>{ from },
                CALL_FUNC,
                uuid,
                std::vector<uint8_t>{}, 
                result
            );
            MessageProcessor::instance().append(MessageProcessor::Send, response);
        });
    // Notify called of incoming call
    auto request = std::make_unique<Message>(
        message->id(), 
        message->type(), 
        message->from(), 
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(), 
        static_cast<int>(timer));
    MessageProcessor::instance().append(MessageProcessor::Send, request);
    // Notify the caller that the called phone is ringing.
    auto response = std::make_unique<Message>(
        message->id(),
        "RSP",
        "server",
        std::vector<std::string>{ from },
        CALL_FUNC,
        message->uuid(),
        std::vector<uint8_t>{},
        result
    );
    MessageProcessor::instance().append(MessageProcessor::Send, response);
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    LogInfo() << "message:" << message->func() << from << "answer call" << to;
    auto timer = message->result();
    TimerManager::instance().remove(timer);
    auto request = std::make_unique<Message>(
        message->id(),
        "RSP",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        0
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void HangupCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    LogInfo() << "message:" << message->func() << from << "hangup call" << to;
    auto timer = message->result();
    TimerManager::instance().remove(timer);
    auto request = std::make_unique<Message>(
        message->id(),
        "RSP",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        0
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}
