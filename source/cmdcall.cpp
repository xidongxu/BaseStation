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
// define by device
#define CALL_STATE_IDLE             0
#define CALL_STATE_LINKING          1
#define CALL_STATE_INCOMING         2
#define CALL_STATE_RINGING          3
#define CALL_STATE_ACTIVE           4
#define CALL_STATE_HANGUP           5
#define CALL_STATE_NO_ANSWER        6
// define by station
#define CALL_STATE_EMPTY_NUMBER     100
#define CALL_STATE_OFFLINE          101

void MakeCall::execute(std::unique_ptr<Message>& message) {
    auto id = message->id();
    auto from = message->from();
    auto to = message->to().at(0);
    auto func = message->func();
    auto uuid = message->uuid();
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
    auto equipment = EquipmentManager::instance().equipment(to);
    // Notify "from" that the called phone number is empty.
    if (!equipment) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            CALL_FUNC,
            uuid,
            message->content(),
            CALL_STATE_EMPTY_NUMBER
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    // Notify "from" that the called phone is offline.
    if (equipment->state() != Equipment::Online) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            CALL_FUNC,
            uuid,
            message->content(),
            CALL_STATE_OFFLINE
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    // Start the answer wait timer.
    auto timer = TimerManager::instance().create();
    TimerManager::instance().start(
        timer, 
        seconds(30), 
        [timer, id, from, func, to, uuid, equipment]() {
            LogInfo() << "timer:" << timer << "timeout";
            TimerManager::instance().remove(timer);
            if (equipment) {
                equipment->removeCall(uuid);
            }
            // Notify "from" no answer
            auto response = std::make_unique<Message>(
                id,
                "REQ",
                "server",
                std::vector<std::string>{ from },
                CALL_FUNC,
                uuid,
                std::vector<uint8_t>{}, 
                CALL_STATE_NO_ANSWER
            );
            MessageProcessor::instance().append(MessageProcessor::Send, response);
            // Notify "to" no answer
            response = std::make_unique<Message>(
                id,
                "REQ",
                "server",
                std::vector<std::string>{ to },
                CALL_FUNC,
                uuid,
                std::vector<uint8_t>{},
                CALL_STATE_NO_ANSWER
            );
            MessageProcessor::instance().append(MessageProcessor::Send, response);
        });
    auto call = std::make_shared<Call>(uuid, timer, from, to);
    equipment->appendCall(call);
    // Notify "from" linking
    auto response = std::make_unique<Message>(
        id,
        "RSP",
        "server",
        std::vector<std::string>{ from },
        CALL_FUNC,
        uuid,
        std::vector<uint8_t>{},
        CALL_STATE_LINKING
    );
    MessageProcessor::instance().append(MessageProcessor::Send, response);
    // Notify "to" incoming
    auto request = std::make_unique<Message>(
        message->id(), 
        message->type(), 
        message->from(), 
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(), 
        CALL_STATE_INCOMING
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RecvCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
    auto equipment = EquipmentManager::instance().equipment(from);
    if (!equipment || equipment->state() != Equipment::Online) {
        LogError() << "equipment:" << to << "not online";
        return;
    }
    auto call = equipment->findCall(uuid);
    if (!call) {
        LogError() << "call:" << to << "has ended or never existed";
        return;
    }
    // Not need delete timer for now
    TimerManager::instance().reset(call->timer());
    // Notify "from" ringing
    auto request = std::make_unique<Message>(
        message->id(),
        "REQ",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        CALL_STATE_RINGING
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RecvRing::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
    auto equipment = EquipmentManager::instance().equipment(from);
    if (!equipment || equipment->state() != Equipment::Online) {
        LogError() << "equipment:" << to << "not online";
        return;
    }
    auto call = equipment->findCall(uuid);
    if (!call) {
        LogError() << "call:" << to << "has ended or never existed";
        return;
    }
    // Delete timer now
    TimerManager::instance().remove(call->timer());
    // Notify "from" active
    auto response = std::make_unique<Message>(
        message->id(),
        "RSP",
        "server",
        std::vector<std::string>{ from },
        CALL_FUNC,
        uuid,
        std::vector<uint8_t>{},
        CALL_STATE_ACTIVE
    );
    MessageProcessor::instance().append(MessageProcessor::Send, response);
    // Notify "to" active
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        CALL_STATE_ACTIVE
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RecvAnswer::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
}

void HangupCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
    auto equipment = EquipmentManager::instance().equipment(to);
    if (!equipment || equipment->state() != Equipment::Online) {
        LogError() << "equipment:" << to << "not online";
        return;
    }
    auto call = equipment->findCall(uuid);
    if (!call) {
        LogError() << "call:" << to << "has ended or never existed";
        return;
    }
    // Delete timer now
    TimerManager::instance().remove(call->timer());
    // Notify "from" hangup
    auto response = std::make_unique<Message>(
        message->id(),
        "RSP",
        "server",
        std::vector<std::string>{ from },
        CALL_FUNC,
        uuid,
        std::vector<uint8_t>{},
        CALL_STATE_HANGUP
    );
    MessageProcessor::instance().append(MessageProcessor::Send, response);
    // Notify "to" hangup
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        CALL_STATE_HANGUP
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RecvHangup::execute(std::unique_ptr<Message>& message) {
    LogInfo() << "\r\n message:" << message->raw().data() << "\r\n";
}
