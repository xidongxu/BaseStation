#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include "command.h"

class SendShortMsg : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};

class RecvShortMsg : public Command {
public:
    void execute(std::unique_ptr<Message>& message) override;
};
