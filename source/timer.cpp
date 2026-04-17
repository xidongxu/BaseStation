#include <chrono>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "processor.h"
#include "timer.h"

#define LOG_TAG "_timer_"
#include "logger.h"

Timer::Timer(asio::io_context& context, uint64_t key) : m_timer(context), m_strander(asio::make_strand(context)), m_key(key) {
    LogInfo() << "timer:" << m_key << "create";
}

Timer::~Timer() {
    stop();
    LogInfo() << "timer:" << m_key << "destory";
}

void Timer::start(std::chrono::milliseconds interval, std::function<void()> callback, bool periodic) {
    auto self = weak_from_this();
    asio::dispatch(
        m_strander,
        [self, interval, callback = std::move(callback), periodic]() {
            auto timer = self.lock();
            if (!timer) {
                return;
            }
            timer->m_interval = interval;
            timer->m_callback = std::move(callback);
            timer->m_periodic = periodic;
            timer->m_running = true;
            timer->schedule();
        });
}

void Timer::stop() {
    auto self = weak_from_this();
    asio::dispatch(
        m_strander,
        [self]() {
            auto timer = self.lock();
            if (!timer) {
                return;
            }
            timer->m_running = false;
            timer->m_timer.cancel();
        });
}

void Timer::reset() {
    auto self = weak_from_this();
    asio::dispatch(
        m_strander,
        [self]() {
            auto timer = self.lock();
            if (!timer) {
                return;
            }
            if (!timer->m_running) {
                return;
            }
            timer->m_timer.cancel();
            timer->schedule();
        });
}

void Timer::schedule() {
    if (!m_running)
        return;
    auto self = weak_from_this();
    m_timer.expires_after(m_interval);
    m_timer.async_wait(
        asio::bind_executor(
            m_strander,
            [self](asio::error_code error) {
                auto timer = self.lock();
                if (!timer) {
                    return;
                }
                if (error || !timer->m_running) {
                    return;
                }
                if (timer->m_callback) {
                    timer->m_callback();
                }
                if (timer->m_periodic) {
                    timer->schedule();
                } else {
                    timer->m_running = false;
                }
            }
        ));
}

TimerManager::TimerManager() : m_context(), m_worker(asio::make_work_guard(m_context)) {
    m_thread = std::thread([this]() {
        m_context.run();
    });
    LogInfo() << "TimerManager start";
}

TimerManager::~TimerManager() {
    m_worker.reset();
    m_context.stop();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_timers.begin(); it != m_timers.end(); ) {
        auto& timer = it->second;
        if (timer) {
            timer->stop();
        }
        it = m_timers.erase(it);
    }
    LogInfo() << "TimerManager stop";
}

uint64_t TimerManager::create() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto key = m_counter;
    auto timer = std::make_shared<Timer>(m_context, key);
    m_timers.insert({ key, timer });
    m_counter = m_counter + 1;
    return key;
}

void TimerManager::start(uint64_t key, std::chrono::milliseconds interval, std::function<void()> callback, bool periodic) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(key);
    if (it != m_timers.end()) {
        it->second->start(interval, std::move(callback), periodic);
    }
}

void TimerManager::stop(uint64_t key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(key);
    if (it != m_timers.end()) {
        it->second->stop();
    }
}

void TimerManager::reset(uint64_t key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(key);
    if (it != m_timers.end()) {
        it->second->reset();
    }
}

bool TimerManager::running(uint64_t key) {
    bool result = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(key);
    if (it != m_timers.end()) {
        result = it->second->running();
    }
    return result;
}

void TimerManager::remove(uint64_t key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(key);
    if (it != m_timers.end()) {
        it->second->stop();
        m_timers.erase(it);
    }
}
