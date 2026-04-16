#include <chrono>
#include <thread>
#include "cJSON.h"
#include "command.h"

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

class Heart : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        auto response = std::make_unique<Message>(
            message->id(),
            "RSP",
            "server",
            std::vector<std::string>{ message->from() },
            "HEART",
            std::vector<uint8_t>{},
            0
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
    }
};

class Offline : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        auto response = std::make_unique<Message>(
            message->id(),
            "RSP",
            "server",
            std::vector<std::string>{ message->from() },
            "OFFLINE",
            std::vector<uint8_t>{},
            0
        );
        MessageProcessor::instance().append(MessageProcessor::Send, response);
    }
};

class Make_Call : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        std::cout << "message:" << message->func() << std::endl;
    }
};

class Answer_Call : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        std::cout << "message:" << message->func() << std::endl;
    }
};

class End_Call : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        std::cout << "message:" << message->func() << std::endl;
    }
};

class Reject_Call : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        std::cout << "message:" << message->func() << std::endl;
    }
};

class Send_Message : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override {
        std::cout << "message:" << message->func() << std::endl;
    }
};

void registerCommands() {
    CommandBuilder::instance().registerCommand(
        "HEART", 
        []() { return std::make_unique<Heart>(); });
    CommandBuilder::instance().registerCommand(
        "OFFLINE",
        []() { return std::make_unique<Offline>(); });
    CommandBuilder::instance().registerCommand(
        "MAKE_CALL", 
        []() { return std::make_unique<Make_Call>(); });
    CommandBuilder::instance().registerCommand(
        "ANSWER_CALL",
        []() { return std::make_unique<Answer_Call>(); });
    CommandBuilder::instance().registerCommand(
        "END_CALL",
        []() { return std::make_unique<End_Call>(); });
    CommandBuilder::instance().registerCommand(
        "REJECT_CALL",
        []() { return std::make_unique<Reject_Call>(); });
    CommandBuilder::instance().registerCommand(
        "SEND_MESSAGE", 
        []() { return std::make_unique<Send_Message>(); });
}
