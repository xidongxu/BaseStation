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
        // No need to respond to heartbeat for now
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
        "RECV_CALL",
        []() { return std::make_unique<RecvCall>(); });
    CommandBuilder::instance().registerCommand(
        "ANSWER_CALL",
        []() { return std::make_unique<AnswerCall>(); });
    CommandBuilder::instance().registerCommand(
        "HANGUP_CALL",
        []() { return std::make_unique<HangupCall>(); });
    CommandBuilder::instance().registerCommand(
        "SEND_MESSAGE", 
        []() { return std::make_unique<SendShortMsg>(); });
    CommandBuilder::instance().registerCommand(
        "RECV_MESSAGE",
        []() { return std::make_unique<RecvShortMsg>(); });
}
