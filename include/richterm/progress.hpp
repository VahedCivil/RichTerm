#pragma once

#include "console.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace richterm {

inline std::string repeat_utf8(std::string_view glyph, std::size_t count) {
    std::string result;
    result.reserve(glyph.size() * count);
    for (std::size_t i = 0; i < count; ++i) result += glyph;
    return result;
}

using TaskId = std::size_t;

inline std::string_view progress_color_name(Color color) noexcept {
    switch (color) {
        case Color::black: return "black"; case Color::red: return "red";
        case Color::green: return "green"; case Color::yellow: return "yellow";
        case Color::blue: return "blue"; case Color::magenta: return "magenta";
        case Color::cyan: return "cyan"; case Color::white: return "white";
        case Color::bright_black: return "bright_black"; case Color::bright_red: return "bright_red";
        case Color::bright_green: return "bright_green"; case Color::bright_yellow: return "bright_yellow";
        case Color::bright_blue: return "bright_blue"; case Color::bright_magenta: return "bright_magenta";
        case Color::bright_cyan: return "bright_cyan"; case Color::bright_white: return "bright_white";
        case Color::default_color: return "white";
    }
    return "green";
}

struct ProgressTask {
    TaskId id{};
    std::string description;
    double completed{0.0};
    std::optional<double> total;
    std::chrono::steady_clock::time_point started{std::chrono::steady_clock::now()};
};

// Progress updates are animated on interactive terminals. For redirected output,
// refresh() prints a stable snapshot and no worker thread is created.
class Progress {
public:
    explicit Progress(Console& console, std::size_t bar_width = 24,
                      bool auto_refresh = true, double refresh_per_second = 12.0)
        : console_(&console),
          bar_width_(std::max<std::size_t>(4, bar_width)),
          auto_refresh_(auto_refresh),
          refresh_interval_(make_interval(refresh_per_second)) {}

    Progress(const Progress&) = delete;
    Progress& operator=(const Progress&) = delete;

    ~Progress() { stop(); }

    TaskId add_task(std::string description, std::optional<double> total = std::nullopt,
                    double completed = 0.0) {
        validate_non_negative(completed, "Progress completed value");
        if (total) validate_non_negative(*total, "Progress total");
        TaskId id{};
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            id = tasks_.size();
            tasks_.push_back({id, std::move(description), completed, total});
        }
        request_refresh();
        return id;
    }

    void update(TaskId id, double completed, std::optional<double> total = std::nullopt,
                std::optional<std::string> description = std::nullopt) {
        validate_non_negative(completed, "Progress completed value");
        if (total) validate_non_negative(*total, "Progress total");
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            auto& task = get_task_unlocked(id);
            task.completed = completed;
            if (total) task.total = total;
            if (description) task.description = std::move(*description);
        }
        request_refresh();
    }

    void advance(TaskId id, double amount = 1.0) {
        if (!std::isfinite(amount))
            throw std::invalid_argument("Progress increment must be finite");
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            auto& task = get_task_unlocked(id);
            const double next = task.completed + amount;
            if (!std::isfinite(next))
                throw std::invalid_argument("Progress completed value overflowed");
            task.completed = std::max(0.0, next);
        }
        request_refresh();
    }

    [[nodiscard]] std::vector<ProgressTask> tasks() const {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        return tasks_;
    }

    [[nodiscard]] std::string render() const {
        return render_at_frame(animation_frame_.load(std::memory_order_relaxed));
    }

    void start() {
        bool expected = false;
        if (!active_.compare_exchange_strong(expected, true)) return;
        if (console_->supports_live_output() && auto_refresh_) {
            stop_requested_.store(false, std::memory_order_release);
            try {
                worker_ = std::thread([this] { worker_loop(); });
            } catch (...) {
                active_.store(false, std::memory_order_release);
                throw;
            }
        } else {
            console_->print(render());
        }
    }

    // With automatic refresh enabled this requests an immediate redraw. When
    // not running interactively, it appends a snapshot for logs and CI output.
    void refresh() {
        if (active_.load(std::memory_order_acquire) && console_->supports_live_output() && auto_refresh_) {
            request_refresh();
            return;
        }
        console_->print(render());
    }

    void stop() noexcept {
        if (!active_.exchange(false, std::memory_order_acq_rel)) return;
        stop_requested_.store(true, std::memory_order_release);
        wake_.notify_all();
        if (worker_.joinable()) worker_.join();
        try {
            if (console_->supports_live_output() && auto_refresh_) draw_live_frame();
            else console_->print(render()); // Preserve the final state in files, pipes and CI logs.
        } catch (...) {
            // Destructors must not throw.
        }
    }

    [[nodiscard]] bool active() const noexcept { return active_.load(std::memory_order_acquire); }
    void set_bar_color(Color color) noexcept { bar_color_.store(color, std::memory_order_relaxed); }
    void set_pulse_color(Color color) noexcept { pulse_color_.store(color, std::memory_order_relaxed); }

private:
    static std::chrono::milliseconds make_interval(double refresh_per_second) {
        if (!std::isfinite(refresh_per_second) || refresh_per_second <= 0.0 || refresh_per_second > 1000.0)
            throw std::invalid_argument("refresh_per_second must be finite and in (0, 1000]");
        const auto ms = static_cast<long long>(std::max(1.0, std::round(1000.0 / refresh_per_second)));
        return std::chrono::milliseconds{ms};
    }

    static void validate_non_negative(double value, const char* label) {
        if (!std::isfinite(value) || value < 0.0)
            throw std::invalid_argument(std::string(label) + " must be finite and non-negative");
    }

    ProgressTask& get_task_unlocked(TaskId id) {
        if (id >= tasks_.size()) throw std::out_of_range("Unknown progress task id");
        return tasks_[id];
    }

    [[nodiscard]] std::string render_at_frame(std::size_t frame) const {
        const auto snapshot = tasks();
        std::string output = "[bold cyan]Progress[/bold cyan]";
        static constexpr std::array<std::string_view, 8> pulse_glyphs{
            "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"
        };
        for (const auto& task : snapshot) {
            output += "\n" + task.description + " ";
            double ratio = 0.0;
            std::size_t filled = 0;
            if (task.total) {
                ratio = (*task.total == 0.0)
                    ? 1.0 : std::clamp(task.completed / *task.total, 0.0, 1.0);
                filled = static_cast<std::size_t>(std::round(ratio * static_cast<double>(bar_width_)));
                const auto bar_color = progress_color_name(bar_color_.load(std::memory_order_relaxed));
                output += "[" + std::string(bar_color) + "]" + repeat_utf8("━", filled) + "[/" + std::string(bar_color) + "]";
                output += "[bright_black]" + repeat_utf8("─", bar_width_ - filled) + "[/bright_black] ";
            } else {
                const std::size_t pos = frame % bar_width_;
                output += "[bright_black]" + repeat_utf8("─", pos) + "[/bright_black]";
                const auto pulse_color = progress_color_name(pulse_color_.load(std::memory_order_relaxed));
                output += "[bold " + std::string(pulse_color) + "]" + std::string(pulse_glyphs[frame % pulse_glyphs.size()]) + "[/bold " + std::string(pulse_color) + "]";
                output += "[bright_black]" + repeat_utf8("─", bar_width_ - pos - 1) + "[/bright_black] ";
            }

            if (task.total) {
                std::ostringstream percent;
                percent << std::fixed << std::setprecision(1) << ratio * 100.0;
                output += "[bold]" + percent.str() + "%[/bold] ";
                output += format("({:.0f}/{:.0f})", task.completed, *task.total);
            } else {
                output += "[bold yellow]working[/bold yellow] ";
                output += format("({:.0f}/?)", task.completed);
            }
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - task.started).count();
            output += format("  [dim]{}s[/dim]", elapsed);
        }
        if (snapshot.empty()) output += "\n[dim]No active tasks[/dim]";
        return output;
    }

    void request_refresh() {
        refresh_requested_.store(true, std::memory_order_release);
        wake_.notify_all();
    }

    void draw_live_frame() {
        std::lock_guard<std::mutex> frame_lock(draw_mutex_);
        const std::size_t frame = animation_frame_.fetch_add(1, std::memory_order_relaxed);
        const std::string text = render_at_frame(frame);
        const std::size_t line_count = static_cast<std::size_t>(
            std::count(text.begin(), text.end(), '\n')) + 1;
        console_->write_live_frame(text, previous_line_count_);
        previous_line_count_ = line_count;
    }

    void worker_loop() noexcept {
        try {
            while (!stop_requested_.load(std::memory_order_acquire)) {
                draw_live_frame();
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
    std::size_t bar_width_;
    std::atomic<Color> bar_color_{Color::green};
    std::atomic<Color> pulse_color_{Color::cyan};
    bool auto_refresh_;
    std::chrono::milliseconds refresh_interval_;
    mutable std::mutex tasks_mutex_;
    std::vector<ProgressTask> tasks_;
    std::atomic<bool> active_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<std::size_t> animation_frame_{0};
    std::atomic<bool> refresh_requested_{false};
    std::thread worker_;
    std::mutex draw_mutex_;
    std::mutex wait_mutex_;
    std::condition_variable wake_;
    std::size_t previous_line_count_{0};
};

} // namespace richterm
