#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#  include <io.h>
#else
#  include <unistd.h>
#endif

namespace richterm {

enum class Color : unsigned char {
    black, red, green, yellow, blue, magenta, cyan, white,
    bright_black, bright_red, bright_green, bright_yellow,
    bright_blue, bright_magenta, bright_cyan, bright_white,
    default_color
};

enum class Style : unsigned char { bold, dim, italic, underline, reverse, strike, reset };
enum class ColorMode : unsigned char { automatic, always, never };

struct BackgroundColor { Color value; };
[[nodiscard]] inline BackgroundColor on(Color color) noexcept { return {color}; }

struct TextStyle {
    std::optional<Color> foreground;
    std::optional<Color> background;
    std::vector<Style> attributes;
    bool reset_first{false};
};

[[nodiscard]] inline TextStyle operator+(Color color, Style style) {
    TextStyle result; result.foreground = color;
    if (style == Style::reset) result.reset_first = true;
    else result.attributes.push_back(style);
    return result;
}
[[nodiscard]] inline TextStyle operator+(Style style, Color color) { return color + style; }
[[nodiscard]] inline TextStyle operator+(Color color, BackgroundColor bg) {
    TextStyle result; result.foreground = color; result.background = bg.value; return result;
}
[[nodiscard]] inline TextStyle operator+(BackgroundColor bg, Color color) { return color + bg; }
[[nodiscard]] inline TextStyle operator+(TextStyle value, Style style) {
    if (style == Style::reset) value.reset_first = true;
    else value.attributes.push_back(style);
    return value;
}
[[nodiscard]] inline TextStyle operator+(TextStyle value, Color color) { value.foreground = color; return value; }
[[nodiscard]] inline TextStyle operator+(TextStyle value, BackgroundColor bg) { value.background = bg.value; return value; }
[[nodiscard]] inline TextStyle operator+(Style style, BackgroundColor bg) {
    TextStyle value; value.background = bg.value;
    if (style == Style::reset) value.reset_first = true;
    else value.attributes.push_back(style);
    return value;
}
[[nodiscard]] inline TextStyle operator+(BackgroundColor bg, Style style) { return style + bg; }

struct StyleState {
    std::optional<Color> foreground;
    std::optional<Color> background;
    std::vector<Style> attributes;
    bool empty() const noexcept { return !foreground && !background && attributes.empty(); }
};

namespace detail {
inline constexpr std::array<int, 17> foreground_codes{
    30,31,32,33,34,35,36,37,90,91,92,93,94,95,96,97,39
};
inline constexpr std::array<int, 17> background_codes{
    40,41,42,43,44,45,46,47,100,101,102,103,104,105,106,107,49
};
inline bool valid_color(Color c) noexcept { return static_cast<unsigned>(c) <= static_cast<unsigned>(Color::default_color); }
inline std::string ansi_code(const StyleState& state) {
    if (state.empty()) return {};
    std::string result = "\x1b[";
    bool first = true;
    auto append = [&](int code) { if (!first) result += ';'; result += std::to_string(code); first = false; };
    if (state.foreground && valid_color(*state.foreground)) append(foreground_codes[static_cast<std::size_t>(*state.foreground)]);
    if (state.background && valid_color(*state.background)) append(background_codes[static_cast<std::size_t>(*state.background)]);
    for (Style style : state.attributes) {
        switch (style) {
            case Style::bold: append(1); break;
            case Style::dim: append(2); break;
            case Style::italic: append(3); break;
            case Style::underline: append(4); break;
            case Style::reverse: append(7); break;
            case Style::strike: append(9); break;
            case Style::reset: break;
        }
    }
    if (first) return {};
    result += 'm';
    return result;
}
inline bool automatic_color_enabled() noexcept {
    if (std::getenv("NO_COLOR") != nullptr) return false;
    if (const char* term = std::getenv("TERM")) if (std::string_view{term} == "dumb") return false;
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return ::isatty(STDOUT_FILENO) != 0;
#endif
}
inline bool color_enabled(ColorMode mode) noexcept {
    if (mode == ColorMode::always) return true;
    if (mode == ColorMode::never) return false;
    return automatic_color_enabled();
}
inline void apply_token(StyleState& state, Color color) { if (valid_color(color)) state.foreground = color; }
inline void apply_token(StyleState& state, Style style) {
    if (style == Style::reset) state = {};
    else state.attributes.push_back(style);
}
inline void apply_token(StyleState& state, BackgroundColor bg) { if (valid_color(bg.value)) state.background = bg.value; }
inline void apply_token(StyleState& state, const TextStyle& style) {
    if (style.reset_first) state = {};
    if (style.foreground) apply_token(state, *style.foreground);
    if (style.background) apply_token(state, BackgroundColor{*style.background});
    for (auto attr : style.attributes) apply_token(state, attr);
}
inline bool contains_style(const StyleState& s, Style style) {
    return std::find(s.attributes.begin(), s.attributes.end(), style) != s.attributes.end();
}
} // namespace detail

[[nodiscard]] inline std::string colorize(std::string_view text, Color foreground,
                                          ColorMode mode = ColorMode::always) {
    if (!detail::color_enabled(mode) || !detail::valid_color(foreground)) return std::string{text};
    StyleState state; state.foreground = foreground;
    return detail::ansi_code(state) + std::string{text} + "\x1b[0m";
}

[[nodiscard]] inline std::string to_ansi(const StyleState& state, std::string_view text,
                                         ColorMode mode = ColorMode::always) {
    if (!detail::color_enabled(mode) || state.empty()) return std::string{text};
    return detail::ansi_code(state) + std::string{text} + "\x1b[0m";
}


[[nodiscard]] inline std::string colorize(std::string_view text,
                                          std::optional<Color> foreground,
                                          std::optional<Color> background,
                                          std::initializer_list<Style> attributes,
                                          ColorMode mode = ColorMode::always) {
    StyleState state;
    state.foreground = foreground;
    state.background = background;
    state.attributes.assign(attributes.begin(), attributes.end());
    return to_ansi(state, text, mode);
}

[[nodiscard]] inline bool terminal_supports_color() noexcept {
    if (std::getenv("NO_COLOR") != nullptr) return false;
    if (const char* term = std::getenv("TERM")) {
        if (std::string_view{term} == "dumb") return false;
    }
#if defined(_WIN32)
    return true; // Windows Terminal and modern consoles support VT; callers may force never.
#else
    return true; // The stream-specific TTY check is performed by Console.
#endif
}

} // namespace richterm
