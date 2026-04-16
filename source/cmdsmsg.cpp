#include <chrono>
#include <thread>
#include "cJSON.h"
#include "cmdsmsg.h"

#define LOG_TAG "cmdsmsg"
#include "logger.h"

using namespace std;

void SendShortMsg::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}

void RecvShortMsg::execute(std::unique_ptr<Message>& message) {
    std::cout << "message:" << message->func() << std::endl;
}
