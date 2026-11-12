#pragma once

#include "format.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <ostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <io.h>
#else
#  include <sys/ioctl.h>
#  include <unistd.h>
#endif

namespace richterm {

class Table;

struct ConsoleOptions {
    ColorMode color{ColorMode::automatic};
    int width{0};
    bool markup{true};
};

namespace detail {
inline bool stream_is_terminal(std::ostream& stream) noexcept {
#if defined(_WIN32)
    if (&stream == &std::cout) return _isatty(_fileno(stdout)) != 0;
    if (&stream == &std::cerr || &stream == &std::clog) return _isatty(_fileno(stderr)) != 0;
    return false;
#else
    if (&stream == &std::cout) return ::isatty(STDOUT_FILENO) != 0;
    if (&stream == &std::cerr || &stream == &std::clog) return ::isatty(STDERR_FILENO) != 0;
    return false;
#endif
}
inline int terminal_width(std::ostream& stream) noexcept {
#if defined(_WIN32)
    if (&stream != &std::cout && &stream != &std::cerr && &stream != &std::clog) return 80;
    DWORD handle_id = (&stream == &std::cerr || &stream == &std::clog) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
    HANDLE handle = GetStdHandle(handle_id);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (handle != INVALID_HANDLE_VALUE && handle != nullptr && GetConsoleScreenBufferInfo(handle, &info))
        return static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
#else
    int fd = -1;
    if (&stream == &std::cout) fd = STDOUT_FILENO;
    else if (&stream == &std::cerr || &stream == &std::clog) fd = STDERR_FILENO;
    if (fd >= 0) {
        winsize size{};
        if (::ioctl(fd, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) return static_cast<int>(size.ws_col);
    }
#endif
    return 80;
}

inline void initialize_windows_console(std::ostream& stream) noexcept {
#if defined(_WIN32)
    if (&stream != &std::cout && &stream != &std::cerr && &stream != &std::clog) return;
    DWORD handle_id = (&stream == &std::cerr || &stream == &std::clog)
        ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
    HANDLE handle = GetStdHandle(handle_id);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) return;
    DWORD mode = 0;
    // Change the output code page only for real console handles. Redirected
    // files/pipes must remain byte-for-byte UTF-8 and must not alter the console.
    if (GetConsoleMode(handle, &mode)) SetConsoleOutputCP(CP_UTF8);
#else
    (void)stream;
#endif
}

inline void enable_windows_vt(std::ostream& stream) noexcept {
#if defined(_WIN32)
    if (&stream != &std::cout && &stream != &std::cerr && &stream != &std::clog) return;
    DWORD handle_id = (&stream == &std::cerr || &stream == &std::clog) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
    HANDLE handle = GetStdHandle(handle_id);
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr) return;
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode)) SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    (void)stream;
#endif
}
inline std::optional<Color> parse_color(std::string_view name) {
    static constexpr std::pair<std::string_view, Color> colors[] = {
        {"black",Color::black},{"red",Color::red},{"green",Color::green},{"yellow",Color::yellow},
        {"blue",Color::blue},{"magenta",Color::magenta},{"cyan",Color::cyan},{"white",Color::white},
        {"bright_black",Color::bright_black},{"bright_red",Color::bright_red},{"bright_green",Color::bright_green},
        {"bright_yellow",Color::bright_yellow},{"bright_blue",Color::bright_blue},
        {"bright_magenta",Color::bright_magenta},{"bright_cyan",Color::bright_cyan},{"bright_white",Color::bright_white},
        {"grey0",Color::bright_black},{"gray0",Color::bright_black}
    };
    for (const auto& [key, value] : colors) if (key == name) return value;
    return std::nullopt;
}
inline bool parse_markup_style(std::string_view tag, StyleState& state) {
    std::istringstream words{std::string(tag)};
    std::string word;
    bool recognized = false;
    bool background_next = false;
    while (words >> word) {
        if (word == "on") { background_next = true; continue; }
        if (word == "bold" || word == "strong") { state.attributes.push_back(Style::bold); recognized = true; continue; }
        if (word == "dim") { state.attributes.push_back(Style::dim); recognized = true; continue; }
        if (word == "italic" || word == "i") { state.attributes.push_back(Style::italic); recognized = true; continue; }
        if (word == "underline" || word == "u") { state.attributes.push_back(Style::underline); recognized = true; continue; }
        if (word == "reverse") { state.attributes.push_back(Style::reverse); recognized = true; continue; }
        if (word == "strike" || word == "strikethrough") { state.attributes.push_back(Style::strike); recognized = true; continue; }
        if (auto color = parse_color(word)) {
            if (background_next) state.background = *color;
            else state.foreground = *color;
            background_next = false;
            recognized = true;
            continue;
        }
        return false;
    }
    return recognized && !background_next;
}
inline bool same_style(const StyleState& a, const StyleState& b) {
    return a.foreground == b.foreground && a.background == b.background && a.attributes == b.attributes;
}
} // namespace detail

class Console {
public:
    explicit Console(std::ostream& stream = std::cout, ConsoleOptions options = {})
        : stream_(&stream), options_(options) {
        if (options_.width <= 0) options_.width = detail::terminal_width(stream);
        detail::initialize_windows_console(stream);
        detail::enable_windows_vt(stream);
    }

    [[nodiscard]] std::ostream& stream() const noexcept { return *stream_; }
    [[nodiscard]] bool is_terminal() const noexcept { return detail::stream_is_terminal(*stream_); }
    [[nodiscard]] bool supports_live_output() const noexcept { return is_terminal(); }
    [[nodiscard]] const ConsoleOptions& options() const noexcept { return options_; }
    void set_color_mode(ColorMode mode) noexcept { options_.color = mode; }
    void set_width(int width) noexcept { if (width > 0) options_.width = width; }
    void set_markup(bool enabled) noexcept { options_.markup = enabled; }
    [[nodiscard]] bool color_enabled() const noexcept {
        if (options_.color == ColorMode::always) return true;
        if (options_.color == ColorMode::never) return false;
        return detail::stream_is_terminal(*stream_) && terminal_supports_color();
    }

    [[nodiscard]] std::string render_markup(std::string_view text, const StyleState& base = {}) const {
        if (!options_.markup) return to_ansi(base, text, effective_color_mode());
        struct Frame { std::string tag; StyleState state; };
        std::vector<Frame> stack;
        stack.push_back({{}, base});
        std::string output;
        std::string plain;
        auto flush = [&] {
            if (!plain.empty()) {
                output += to_ansi(stack.back().state, plain, effective_color_mode());
                plain.clear();
            }
        };
        for (std::size_t i = 0; i < text.size();) {
            if (text[i] == '[' && i + 1 < text.size() && text[i + 1] == '[') {
                plain += '['; i += 2; continue;
            }
            if (text[i] != '[') { plain += text[i++]; continue; }
            const auto close = text.find(']', i + 1);
            if (close == std::string_view::npos) { plain += text[i++]; continue; }
            auto tag = text.substr(i + 1, close - i - 1);
            if (!tag.empty() && tag.front() == '/') {
                const auto closing = tag.substr(1);
                if (stack.size() > 1 && stack.back().tag == closing) {
                    flush(); stack.pop_back(); i = close + 1; continue;
                }
                plain.append(text.substr(i, close - i + 1)); i = close + 1; continue;
            }
            StyleState next = stack.back().state;
            if (detail::parse_markup_style(tag, next)) {
                flush(); stack.push_back({std::string(tag), std::move(next)}); i = close + 1;
            } else {
                plain.append(text.substr(i, close - i + 1)); i = close + 1;
            }
        }
        flush();
        // Unclosed opening tags are harmless: all spans were independently reset.
        return output;
    }

    void write(std::string_view text) {
        const std::string rendered = render_markup(text);
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->write(rendered.data(), static_cast<std::streamsize>(rendered.size()));
        stream_->flush();
    }
    void print(std::string_view text) {
        const std::string rendered = render_markup(text);
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->write(rendered.data(), static_cast<std::streamsize>(rendered.size()));
        *stream_ << '\n';
        stream_->flush();
    }
    void println(std::string_view text) { print(text); }
    void print(const Table& table);
    void println(const Table& table);

    void rule(std::string_view title = {}) {
        const std::size_t width = static_cast<std::size_t>(options_.width);
        const auto line = [](std::size_t count) {
            std::string value;
            for (std::size_t i = 0; i < count; ++i) value += "─";
            return value;
        };
        if (title.empty()) {
            print("[bright_black]" + line(width) + "[/bright_black]");
            return;
        }
        const std::string label = " " + std::string(title) + " ";
        const std::size_t label_width = label.size();
        const std::size_t remaining = width > label_width ? width - label_width : 2;
        const std::size_t left = remaining / 2;
        const std::size_t right = remaining - left;
        print("[bright_black]" + line(left) + "[/bright_black]" +
              "[bold cyan]" + label + "[/bold cyan]" +
              "[bright_black]" + line(right) + "[/bright_black]");
    }

    // Rewrite a single terminal line. The caller should use this only while
    // supports_live_output() is true; redirected streams should use print().
    void write_live_line(std::string_view markup, bool finish_line = false) {
        const std::string rendered = render_markup(markup);
        std::lock_guard<std::mutex> lock(output_mutex_);
        *stream_ << "\r\x1b[2K";
        stream_->write(rendered.data(), static_cast<std::streamsize>(rendered.size()));
        if (finish_line) *stream_ << '\n';
        stream_->flush();
    }

    // Rewrite a previously emitted multi-line frame. previous_line_count is the
    // number of lines written by the previous frame (not including older output).
    void write_live_frame(std::string_view markup, std::size_t previous_line_count) {
        std::vector<std::string> rendered_lines;
        std::size_t begin = 0;
        for (;;) {
            const auto end = markup.find('\n', begin);
            const auto line = markup.substr(begin, end == std::string_view::npos
                ? markup.size() - begin : end - begin);
            rendered_lines.push_back(render_markup(line));
            if (end == std::string_view::npos) break;
            begin = end + 1;
        }
        if (rendered_lines.empty()) rendered_lines.emplace_back();

        std::lock_guard<std::mutex> lock(output_mutex_);
        if (previous_line_count > 0) {
            *stream_ << "\x1b[" << previous_line_count << 'A';
        }
        for (const auto& line : rendered_lines) {
            *stream_ << "\r\x1b[2K";
            stream_->write(line.data(), static_cast<std::streamsize>(line.size()));
            *stream_ << '\n';
        }
        stream_->flush();
    }

    template <typename... Args>
        requires (sizeof...(Args) > 0)
    void print(std::string_view format_string, Args&&... args) {
        const auto segments = format_segments(format_string, std::forward<Args>(args)...);
        std::string rendered;
        for (const auto& segment : segments) rendered += render_markup(segment.text, segment.style);
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->write(rendered.data(), static_cast<std::streamsize>(rendered.size()));
        *stream_ << '\n';
        stream_->flush();
    }
    template <typename... Args>
        requires (sizeof...(Args) > 0)
    void write(std::string_view format_string, Args&&... args) {
        const auto segments = format_segments(format_string, std::forward<Args>(args)...);
        std::string rendered;
        for (const auto& segment : segments) rendered += render_markup(segment.text, segment.style);
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->write(rendered.data(), static_cast<std::streamsize>(rendered.size()));
        stream_->flush();
    }
    template <typename... Args>
        requires (sizeof...(Args) > 0)
    void println(std::string_view format_string, Args&&... args) {
        print(format_string, std::forward<Args>(args)...);
    }

    void print_raw(std::string_view text) {
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->write(text.data(), static_cast<std::streamsize>(text.size()));
        *stream_ << '\n';
        stream_->flush();
    }
    void flush() {
        std::lock_guard<std::mutex> lock(output_mutex_);
        stream_->flush();
    }

private:
    [[nodiscard]] ColorMode effective_color_mode() const noexcept {
        return color_enabled() ? ColorMode::always : ColorMode::never;
    }
    std::ostream* stream_;
    ConsoleOptions options_;
    mutable std::mutex output_mutex_;
};

inline Console& default_console() {
    static Console console;
    return console;
}

template <typename... Args>
void print(std::string_view format_string, Args&&... args) {
    default_console().print(format_string, std::forward<Args>(args)...);
}
inline void print(std::string_view text) { default_console().print(text); }
template <typename... Args>
void println(std::string_view format_string, Args&&... args) {
    default_console().print(format_string, std::forward<Args>(args)...);
}
inline void println(std::string_view text) { default_console().print(text); }

} // namespace richterm
