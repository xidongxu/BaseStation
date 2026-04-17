#include <chrono>
#include <thread>
#include "cJSON.h"
#include "command.h"
#include "processor.h"
#include "timer.h"

#define LOG_TAG "_timer_"
#include "logger.h"

Timer::Timer(asio::io_context& context) : m_timer(context), m_strander(asio::make_strand(context)) {}

Timer::~Timer() {
    stop();
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

bool Timer::running() const {
    return m_running.load(std::memory_order_relaxed);
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

TimerManager::TimerManager() {
    m_thread = std::thread([this]() {
        m_context.run();
    });
}

TimerManager::~TimerManager() {
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
}

uint64_t TimerManager::append(std::chrono::milliseconds interval, std::function<void()> callback, bool periodic) {
    auto timer = std::make_shared<Timer>(m_context);
    std::lock_guard<std::mutex> lock(m_mutex);
    timer->start(interval, std::move(callback), periodic);
    m_timers.insert({ m_counter, timer });
    m_counter = m_counter + 1;
    return m_counter;
}

void TimerManager::remove(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(id);
    if (it != m_timers.end()) {
        it->second->stop();
        m_timers.erase(it);
    }
}

void TimerManager::stop(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(id);
    if (it != m_timers.end()) {
        it->second->stop();
    }
}

void TimerManager::reset(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(id);
    if (it != m_timers.end()) {
        it->second->reset();
    }
}

bool TimerManager::running(uint64_t id) {
    bool result = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_timers.find(id);
    if (it != m_timers.end()) {
        result = it->second->running();
    }
    return result;
}
