#include <chrono>
#include <memory>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "cmdsmsg.h"
#include "equipment.h"

#define LOG_TAG "cmdsmsg"
#include "logger.h"

using namespace std;

#define SMSG_FUNC                   "SMSG_NOTIFY"
// define by device
#define SMSG_STATE_IDLE             0
#define SMSG_STATE_INCOMING         1
#define SMSG_STATE_RECIVED          2
// define by station
#define SMSG_STATE_EMPTY_NUMBER     100
#define SMSG_STATE_OFFLINE          101
#define SMSG_STATE_IMS_REJECTED     102

void SendShortMsg::execute(std::unique_ptr<Message>& message) {
    auto id = message->id();
    auto from = message->from();
    auto to = message->to().at(0);
    auto func = message->func();
    auto uuid = message->uuid();
    LogInfo() << "message:" << "\r\n" << message->details() << "\r\n";
    // Notify "from" that the sendto phone number is same with "from" phone number.
    if (from == to) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            SMSG_FUNC,
            uuid,
            message->content(),
            SMSG_STATE_IMS_REJECTED
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    auto equipment = EquipmentManager::instance().equipment(to);
    // Notify "from" that the sendto phone number is empty.
    if (!equipment) {
        auto response = std::make_unique<Message>(
            id,
            "RSP",
            "server",
            std::vector<std::string>{ from },
            SMSG_FUNC,
            uuid,
            message->content(),
            SMSG_STATE_EMPTY_NUMBER
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
        return;
    }
    // Notify "sendto" incoming
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        SMSG_FUNC,
        message->uuid(),
        message->content(),
        SMSG_STATE_INCOMING
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}

void RecvShortMsg::execute(std::unique_ptr<Message>& message) {
    auto from = message->from();
    auto to = message->to().at(0);
    auto uuid = message->uuid();
    LogInfo() << "message:" << "\r\n" << message->details() << "\r\n";
    auto equipment = EquipmentManager::instance().equipment(to);
    if (!equipment || equipment->state() != Equipment::Online) {
        LogError() << "equipment:" << to << "not online";
        return;
    }
    // Notify "from" message arrived
    auto request = std::make_unique<Message>(
        message->id(),
        message->type(),
        message->from(),
        message->to(),
        SMSG_FUNC,
        message->uuid(),
        message->content(),
        SMSG_STATE_RECIVED
    );
    MessageProcessor::instance().append(MessageProcessor::Send, request);
}
