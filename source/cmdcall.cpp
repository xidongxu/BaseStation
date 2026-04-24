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
#define CALL_STATE_IDLE             1               ///< call is idle
#define CALL_STATE_MO_CALLING       2               ///< the call setup has been started
#define CALL_STATE_MO_CONN          3               ///< the call is in progress
#define CALL_STATE_MO_ALERT         4               ///< an alert indication has been received
#define CALL_STATE_MT_ALERT         5               ///< an alert indication has been sent
#define CALL_STATE_ACTIVE           6               ///< the connection is established
#define CALL_STATE_MO_REL           7               ///< an outgoing (MO) call is released
#define CALL_STATE_MT_REL           8               ///< an incoming (MT) call is released
#define CALL_STATE_USER_BUSY        9               ///< user is busy
#define CALL_STATE_USER_HANGUP      10              ///< User Determined User Busy
#define CALL_STATE_MO_WAITING       11              ///< Call Waiting (MO)
#define CALL_STATE_MT_WAITING       12              ///< Call Waiting (MT)
#define CALL_STATE_MO_HOLDING       13              ///< MO side holding call
#define CALL_STATE_MT_HOLDING       14              ///< MT side holding call
#define CALL_STATE_MT_CONN          15              ///< imcoming call trigger,used by IMS
// define by station
#define CALL_STATE_EMPTY_NUMBER     100
#define CALL_STATE_OFFLINE          101
#define CALL_STATE_SHUTDOWN         102
#define CALL_STATE_NO_ANSWER        103

void MakeCall::execute(std::unique_ptr<Message>& message) {
    auto id = message->id();
    auto from = message->from();
    auto to = message->to().at(0);
    auto func = message->func();
    auto uuid = message->uuid();
    LogInfo() << "message:" << message->func() << from << "to" << to;
    auto equipment = EquipmentManager::instance().equipment(to);
    // Notify caller that the called phone number is empty.
    auto result = CALL_STATE_EMPTY_NUMBER;
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
        result = equipment->state() == Equipment::Offline ? CALL_STATE_OFFLINE : CALL_STATE_SHUTDOWN;
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
        seconds(10), 
        [timer, id, from, func, uuid, equipment]() {
            LogInfo() << "timer:" << timer << "timeout";
            TimerManager::instance().remove(timer);
            if (equipment) {
                equipment->removeCall(uuid);
            }
            auto result = CALL_STATE_NO_ANSWER;
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
    auto call = std::make_shared<Call>(uuid, timer, from, to);
    equipment->appendCall(call);
    // Notify called of incoming call
    auto request = std::make_unique<Message>(
        message->id(), 
        message->type(), 
        message->from(), 
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(), 
        CALL_STATE_MO_CONN
    );
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
        CALL_STATE_MO_ALERT
    );
    MessageProcessor::instance().append(MessageProcessor::Send, response);
}

void RecvCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "message:" << message->func() << from << "to" << to;
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
    TimerManager::instance().remove(call->timer());
    auto request = std::make_unique<Message>(
        message->id(),
        "RSP",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        CALL_STATE_MT_ALERT
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "message:" << message->func() << from << "to" << to;
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
    TimerManager::instance().remove(call->timer());
    auto request = std::make_unique<Message>(
        message->id(),
        "RSP",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        CALL_STATE_ACTIVE
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void HangupCall::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "message:" << message->func() << from << "to" << to;
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
    TimerManager::instance().remove(call->timer());
    auto result = (from == call->caller()) ? CALL_STATE_MO_REL : CALL_STATE_MT_REL;
    auto request = std::make_unique<Message>(
        message->id(),
        "RSP",
        message->from(),
        message->to(),
        CALL_FUNC,
        message->uuid(),
        message->content(),
        result
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}
