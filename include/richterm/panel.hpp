#pragma once

#include "table.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace richterm {

class Panel {
public:
    explicit Panel(std::string content = {}, std::string title = {})
        : content_(std::move(content)), title_(std::move(title)) {}

    Panel& set_content(std::string content) { content_ = std::move(content); return *this; }
    Panel& set_title(std::string title) { title_ = std::move(title); return *this; }
    Panel& set_width(std::size_t width) noexcept { width_ = width; return *this; }
    Panel& set_padding(std::size_t padding) noexcept { padding_ = padding; return *this; }
    Panel& set_border_style(std::string style) { border_style_ = std::move(style); return *this; }
    Panel& set_title_style(std::string style) { title_style_ = std::move(style); return *this; }
    Panel& set_content_style(std::string style) { content_style_ = std::move(style); return *this; }
    Panel& set_alignment(Alignment alignment) noexcept { alignment_ = alignment; return *this; }

    [[nodiscard]] std::string render() const {
        const auto source_lines = detail::split_lines(content_);
        std::size_t natural_width = detail::display_width(title_);
        for (const auto& line : source_lines)
            natural_width = std::max(natural_width, detail::display_width(line));
        const std::size_t maximum = std::numeric_limits<std::size_t>::max();
        if (padding_ > (maximum - 3) / 2)
            throw std::length_error("Panel padding is too large");
        const std::size_t padding_width = padding_ * 2;
        const std::size_t content_minimum = std::max<std::size_t>(natural_width, 1);
        if (content_minimum > maximum - padding_width - 2)
            throw std::length_error("Panel content width is too large");
        const std::size_t minimum_total = content_minimum + padding_width + 2;
        const std::size_t title_width = detail::display_width(title_);
        if (title_width > maximum - 4)
            throw std::length_error("Panel title is too wide");
        const std::size_t title_minimum = title_width + 4;
        const std::size_t total_width = width_ > 0
            ? std::max({width_, title_minimum, padding_width + 3})
            : std::max({minimum_total, title_minimum, padding_width + 3});
        const std::size_t inner_width = total_width - 2;
        const std::size_t content_width = inner_width > padding_ * 2 ? inner_width - padding_ * 2 : 1;

        std::vector<std::string> lines;
        for (const auto& source : source_lines) {
            auto wrapped = detail::wrap_cell(source, content_width);
            lines.insert(lines.end(), wrapped.begin(), wrapped.end());
        }
        if (lines.empty()) lines.emplace_back();

        std::string output;
        const std::string top_fill = detail::repeat("─", inner_width);
        if (title_.empty()) {
            output += "[" + border_style_ + "]┌" + top_fill + "┐[/" + border_style_ + "]\n";
        } else {
            const std::string label = " " + title_ + " ";
            const std::size_t label_width = std::min(detail::display_width(label), inner_width);
            const std::size_t remaining = inner_width - label_width;
            const std::size_t left = remaining / 2;
            const std::size_t right = remaining - left;
            output += "[" + border_style_ + "]┌" + detail::repeat("─", left) + "[/" + border_style_ + "]";
            output += "[" + title_style_ + "]" + label + "[/" + title_style_ + "]";
            output += "[" + border_style_ + "]" + detail::repeat("─", right) + "┐[/" + border_style_ + "]\n";
        }

        for (const auto& line : lines) {
            const std::string padded = detail::pad_cell(line, content_width, alignment_);
            const std::string body = detail::repeat(" ", padding_) + padded + detail::repeat(" ", padding_);
            const std::string styled = content_style_.empty() ? body : "[" + content_style_ + "]" + body + "[/" + content_style_ + "]";
            output += "[" + border_style_ + "]│[/" + border_style_ + "]" + styled + "[" + border_style_ + "]│[/" + border_style_ + "]\n";
        }
        output += "[" + border_style_ + "]└" + detail::repeat("─", inner_width) + "┘[/" + border_style_ + "]";
        return output;
    }

private:
    std::string content_;
    std::string title_;
    std::string border_style_{"bright_black"};
    std::string title_style_{"bold cyan"};
    std::string content_style_;
    std::size_t width_{0};
    std::size_t padding_{1};
    Alignment alignment_{Alignment::left};
};

inline void print(Console& console, const Panel& panel) { console.print(panel.render()); }
inline void Console::print(const Panel& panel) { print(panel.render()); }
inline void Console::println(const Panel& panel) { print(panel); }

} // namespace richterm
