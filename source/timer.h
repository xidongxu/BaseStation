#pragma once

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>

class Timer : public std::enable_shared_from_this<Timer> {
public:
    explicit Timer(asio::io_context& context);
    ~Timer();
    void start(std::chrono::milliseconds interval, std::function<void()> callback, bool periodic = false);
    void stop();
    void reset();
    bool running() const;

private:
    void schedule();

private:
    asio::steady_timer m_timer;
    asio::strand<asio::io_context::executor_type> m_strander;
    std::chrono::milliseconds m_interval{ 0 };
    std::function<void()> m_callback{};
    std::atomic<bool> m_running{};
    std::atomic<bool> m_periodic{};
};

class TimerManager {
public:
    static TimerManager& instance() {
        static TimerManager instance;
        return instance;
    }
    uint64_t append(std::chrono::milliseconds interval, std::function<void()> callback, bool periodic = false);
    void remove(uint64_t id);
    void stop(uint64_t id);
    void reset(uint64_t id);
    bool running(uint64_t id);

private:
    TimerManager();
    ~TimerManager();
    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;
    TimerManager(TimerManager&&) = delete;
    TimerManager& operator=(TimerManager&&) = delete;

private:
    uint64_t m_counter{};
    std::mutex m_mutex{};
    std::thread m_thread{};
    asio::io_context m_context{};
    std::unordered_map<uint64_t, std::shared_ptr<Timer>> m_timers{};
};
