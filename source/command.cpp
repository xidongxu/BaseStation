#include <chrono>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "cmdcall.h"
#include "cmdsmsg.h"

#define LOG_TAG "command"
#include "logger.h"

using namespace std;

void CommandBuilder::registerCommand(const std::string& name, Creator creator) {
    creators[name] = creator;
}

std::unique_ptr<Command> CommandBuilder::build(const std::string& name) {
    auto it = creators.find(name);
    if (it != creators.end()) {
        return it->second();
    }
    return nullptr;
}

class HeartAndOffline: public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        auto response = std::make_unique<Message>(
            message->id(),
            "RSP",
            "server",
            std::vector<std::string>{ message->from() },
            message->func(),
            std::vector<uint8_t>{},
            0
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
    }
};

void registerCommands() {
    CommandBuilder::instance().registerCommand(
        "HEART", 
        []() { return std::make_unique<HeartAndOffline>(); });
    CommandBuilder::instance().registerCommand(
        "OFFLINE",
        []() { return std::make_unique<HeartAndOffline>(); });
    CommandBuilder::instance().registerCommand(
        "MAKE_CALL", 
        []() { return std::make_unique<MakeCall>(); });
    CommandBuilder::instance().registerCommand(
        "ANSWER_CALL",
        []() { return std::make_unique<AnswerCall>(); });
    CommandBuilder::instance().registerCommand(
        "END_CALL",
        []() { return std::make_unique<EndCall>(); });
    CommandBuilder::instance().registerCommand(
        "REJECT_CALL",
        []() { return std::make_unique<RejectCall>(); });
    CommandBuilder::instance().registerCommand(
        "SEND_MESSAGE", 
        []() { return std::make_unique<SendShortMsg>(); });
    CommandBuilder::instance().registerCommand(
        "RECV_MESSAGE",
        []() { return std::make_unique<RecvShortMsg>(); });
}
