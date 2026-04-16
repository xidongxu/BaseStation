#include <chrono>
#include <thread>
#include "cJSON.h"
#include "cmdcall.h"

#define LOG_TAG "command"
#include "logger.h"

using namespace std;

void MakeCall::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}

void AnswerCall::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}

void EndCall::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}

void RejectCall::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}
