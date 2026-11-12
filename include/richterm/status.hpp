#pragma once

#include "console.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace richterm {

// Status animates a spinner on interactive terminals. When output is redirected,
// it emits ordinary log lines and does not start a worker thread.
class Status {
public:
    explicit Status(Console& console, std::string message,
                   double refresh_per_second = 10.0)
        : console_(&console), message_(std::move(message)),
          refresh_interval_(make_interval(refresh_per_second)) {}

    Status(const Status&) = delete;
    Status& operator=(const Status&) = delete;

    ~Status() { stop(); }

    void start() {
        bool expected = false;
        if (!active_.compare_exchange_strong(expected, true)) return;
        if (console_->supports_live_output()) {
            stop_requested_.store(false, std::memory_order_release);
            try {
                worker_ = std::thread([this] { worker_loop(); });
            } catch (...) {
                active_.store(false, std::memory_order_release);
                throw;
            }
        } else {
            console_->print("[bold cyan]●[/bold cyan] " + message());
        }
    }

    void update(std::string message) {
        {
            std::lock_guard<std::mutex> lock(message_mutex_);
            message_ = std::move(message);
        }
        if (!active_.load(std::memory_order_acquire)) return;
        if (console_->supports_live_output()) {
            refresh_requested_.store(true, std::memory_order_release);
            wake_.notify_all();
        }
        else console_->print("[bold cyan]●[/bold cyan] " + this->message());
    }

    void stop(std::string final_message = {}) noexcept {
        if (!active_.exchange(false, std::memory_order_acq_rel)) return;
        stop_requested_.store(true, std::memory_order_release);
        wake_.notify_all();
        if (worker_.joinable()) worker_.join();
        try {
            if (final_message.empty()) final_message = message();
            if (console_->supports_live_output())
                console_->write_live_line("[bold green]✓[/bold green] " + final_message, true);
            else
                console_->print("[bold green]✓[/bold green] " + final_message);
        } catch (...) {
            // A status indicator must not allow an output exception to escape a destructor.
        }
    }

    [[nodiscard]] bool active() const noexcept { return active_.load(std::memory_order_acquire); }
    [[nodiscard]] std::string message() const {
        std::lock_guard<std::mutex> lock(message_mutex_);
        return message_;
    }

private:
    static std::chrono::milliseconds make_interval(double refresh_per_second) {
        if (!std::isfinite(refresh_per_second) || refresh_per_second <= 0.0 || refresh_per_second > 1000.0)
            throw std::invalid_argument("refresh_per_second must be finite and in (0, 1000]");
        const auto ms = static_cast<long long>(std::max(1.0, std::round(1000.0 / refresh_per_second)));
        return std::chrono::milliseconds{ms};
    }

    void worker_loop() noexcept {
        static constexpr std::array<std::string_view, 10> frames{
            "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
        };
        try {
            std::size_t index = 0;
            while (!stop_requested_.load(std::memory_order_acquire)) {
                const std::string line = "[bold cyan]" + std::string(frames[index % frames.size()]) +
                                         "[/bold cyan] " + message();
                console_->write_live_line(line, false);
                ++index;
                std::unique_lock<std::mutex> lock(wait_mutex_);
                wake_.wait_for(lock, refresh_interval_, [this] {
                    return stop_requested_.load(std::memory_order_acquire) ||
                           refresh_requested_.exchange(false, std::memory_order_acq_rel);
                });
            }
        } catch (...) {
            stop_requested_.store(true, std::memory_order_release);
        }
    }

    Console* console_;
    mutable std::mutex message_mutex_;
    std::string message_;
    std::chrono::milliseconds refresh_interval_;
    std::atomic<bool> active_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<bool> refresh_requested_{false};
    std::thread worker_;
    std::mutex wait_mutex_;
    std::condition_variable wake_;
};

} // namespace richterm
