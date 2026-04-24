#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include "command.h"

class MakeCall : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};

class RecvCall : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};

class AnswerCall : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};

class HangupCall : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};
