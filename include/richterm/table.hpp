#pragma once

#include "console.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace richterm {

enum class Alignment { left, center, right };

struct TableColumn {
    std::string header;
    Alignment alignment{Alignment::left};
    std::size_t min_width{0};
};

namespace detail {
inline std::string strip_markup(std::string_view text) {
    std::string result;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '[' && i + 1 < text.size() && text[i + 1] == '[') { result += '['; i += 2; continue; }
        if (text[i] == '[') {
            const auto end = text.find(']', i + 1);
            if (end != std::string_view::npos) {
                auto tag = text.substr(i + 1, end - i - 1);
                if (!tag.empty() && tag.front() == '/') tag.remove_prefix(1);
                StyleState probe;
                if (parse_markup_style(tag, probe)) { i = end + 1; continue; }
            }
        }
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            std::size_t end = i + 2;
            while (end < text.size() && (text[end] < '@' || text[end] > '~')) ++end;
            if (end < text.size()) { i = end + 1; continue; }
        }
        result += text[i++];
    }
    return result;
}
inline std::size_t display_width(std::string_view text) {
    const std::string plain = strip_markup(text);
    std::size_t count = 0;
    for (char raw : plain) {
        const auto byte = static_cast<unsigned char>(raw);
        if ((byte & 0xC0u) != 0x80u) ++count;
    }
    return count;
}
inline std::string pad_cell(std::string cell, std::size_t width, Alignment alignment) {
    const std::size_t length = display_width(cell);
    if (length >= width) return cell;
    const std::size_t padding = width - length;
    if (alignment == Alignment::right) return std::string(padding, ' ') + cell;
    if (alignment == Alignment::center) {
        const std::size_t left = padding / 2;
        return std::string(left, ' ') + cell + std::string(padding - left, ' ');
    }
    return cell + std::string(padding, ' ');
}
inline std::string repeat(std::string_view value, std::size_t count) {
    std::string result;
    result.reserve(value.size() * count);
    for (std::size_t i = 0; i < count; ++i) result += value;
    return result;
}
} // namespace detail

class Table {
public:
    explicit Table(std::string title = {}) : title_(std::move(title)) {}

    Table& add_column(std::string header, Alignment alignment = Alignment::left,
                      std::size_t min_width = 0) {
        columns_.push_back({std::move(header), alignment, min_width});
        for (auto& row : rows_) row.emplace_back();
        return *this;
    }
    Table& add_row(std::vector<std::string> cells) {
        if (columns_.empty()) {
            for (std::size_t i = 0; i < cells.size(); ++i) columns_.push_back({{}, Alignment::left, 0});
        }
        if (cells.size() != columns_.size()) throw std::invalid_argument("Table row cell count must match the number of columns");
        rows_.push_back(std::move(cells));
        return *this;
    }
    template <typename... Args>
        requires (sizeof...(Args) > 0)
    Table& add_row(Args&&... cells) {
        std::vector<std::string> values;
        values.reserve(sizeof...(Args));
        (values.push_back(richterm::format("{}", std::forward<Args>(cells))), ...);
        return add_row(std::move(values));
    }
    Table& set_caption(std::string caption) { caption_ = std::move(caption); return *this; }
    Table& set_header_style(std::string style) { header_style_ = std::move(style); return *this; }
    Table& set_border_style(std::string style) { border_style_ = std::move(style); return *this; }
    Table& set_row_styles(std::string even_style, std::string odd_style) {
        even_row_style_ = std::move(even_style); odd_row_style_ = std::move(odd_style); return *this;
    }
    Table& show_lines(bool enabled = true) noexcept { show_lines_ = enabled; return *this; }
    Table& show_header(bool enabled = true) noexcept { show_header_ = enabled; return *this; }

    [[nodiscard]] std::size_t row_count() const noexcept { return rows_.size(); }
    [[nodiscard]] std::size_t column_count() const noexcept { return columns_.size(); }

    [[nodiscard]] std::string render() const {
        if (columns_.empty()) return title_.empty() ? std::string{} : title_;
        std::vector<std::size_t> widths(columns_.size(), 0);
        for (std::size_t c = 0; c < columns_.size(); ++c) {
            widths[c] = std::max(columns_[c].min_width, detail::display_width(columns_[c].header));
        }
        for (const auto& row : rows_) {
            for (std::size_t c = 0; c < row.size(); ++c) widths[c] = std::max(widths[c], detail::display_width(row[c]));
        }
        for (auto& width : widths) width = std::max<std::size_t>(width, 1);

        std::string output;
        if (!title_.empty()) output += "[bold]" + title_ + "[/bold]\n";
        auto border = [&](std::string_view left, std::string_view join, std::string_view right, std::string_view horizontal) {
            std::string line = std::string(left);
            for (std::size_t c = 0; c < widths.size(); ++c) {
                line += detail::repeat(horizontal, widths[c] + 2);
                line += (c + 1 == widths.size()) ? std::string(right) : std::string(join);
            }
            return "[" + border_style_ + "]" + line + "[/" + border_style_ + "]\n";
        };
        auto render_row = [&](const std::vector<std::string>& cells, bool header, std::size_t row_index) {
            std::string line = "[" + border_style_ + "]┃[/" + border_style_ + "]";
            for (std::size_t c = 0; c < widths.size(); ++c) {
                const std::string cell = detail::pad_cell(cells[c], widths[c], columns_[c].alignment);
                std::string content = cell;
                const std::string& row_style = header ? header_style_ : (row_index % 2 == 0 ? even_row_style_ : odd_row_style_);
                if (!row_style.empty()) content = "[" + row_style + "]" + content + "[/" + row_style + "]";
                line += " " + content + " ";
                line += "[" + border_style_ + "]┃[/" + border_style_ + "]";
            }
            output += line + '\n';
        };

        output += border("┏", "┳", "┓", "━");
        if (show_header_) {
            std::vector<std::string> headers;
            for (const auto& col : columns_) headers.push_back(col.header);
            render_row(headers, true, 0);
            output += border("┣", "╋", "┫", "━");
        }
        for (std::size_t r = 0; r < rows_.size(); ++r) {
            render_row(rows_[r], false, r);
            if (show_lines_ && r + 1 < rows_.size()) output += border("┣", "╋", "┫", "┄");
        }
        output += border("┗", "┻", "┛", "━");
        if (!caption_.empty()) output += "[dim]" + caption_ + "[/dim]\n";
        if (!output.empty() && output.back() == '\n') output.pop_back();
        return output;
    }

private:
    std::string title_;
    std::string caption_;
    std::string header_style_{"bold cyan"};
    std::string border_style_{"bright_black"};
    std::string even_row_style_{};
    std::string odd_row_style_{};
    bool show_lines_{false};
    bool show_header_{true};
    std::vector<TableColumn> columns_;
    std::vector<std::vector<std::string>> rows_;
};

inline void print(Console& console, const Table& table) { console.print(table.render()); }
inline void Console::print(const Table& table) { print(table.render()); }
inline void Console::println(const Table& table) { print(table); }

} // namespace richterm
