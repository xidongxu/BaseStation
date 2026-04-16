#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <functional>
#include <unordered_map>
#include "processor.h"

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(std::unique_ptr<Message> &message) = 0;
};

class CommandBuilder {
public:
    using Creator = std::function<std::unique_ptr<Command>()>;
    static CommandBuilder& instance() {
        static CommandBuilder instance;
        return instance;
    }
    void registerCommand(const std::string& name, Creator creator);
    std::unique_ptr<Command> build(const std::string& name);

private:
    std::unordered_map<std::string, Creator> creators;
};

void registerCommands();