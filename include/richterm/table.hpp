#pragma once

#include "console.hpp"
#include "unicode.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>
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
    std::vector<std::string> open_tags;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '\\' && i + 1 < text.size() && (text[i + 1] == '[' || text[i + 1] == ']')) {
            result += text[i + 1]; i += 2; continue;
        }
        if (text[i] == '[' && i + 1 < text.size() && text[i + 1] == '[') {
            result += '['; i += 2; continue;
        }
        if (text[i] == ']' && i + 1 < text.size() && text[i + 1] == ']') {
            result += ']'; i += 2; continue;
        }
        if (text[i] == '[') {
            const auto end = text.find(']', i + 1);
            if (end != std::string_view::npos) {
                const auto tag = text.substr(i + 1, end - i - 1);
                if (!tag.empty() && tag.front() == '/') {
                    const auto closing = tag.substr(1);
                    if (!open_tags.empty() && (closing.empty() || open_tags.back() == closing)) {
                        open_tags.pop_back(); i = end + 1; continue;
                    }
                } else {
                    StyleState probe;
                    if (parse_markup_style(tag, probe)) {
                        open_tags.emplace_back(tag); i = end + 1; continue;
                    }
                }
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
    return utf8_width(strip_markup(text));
}
inline std::vector<std::string> split_lines(std::string_view text) {
    std::vector<std::string> lines;
    std::size_t begin = 0;
    for (;;) {
        const auto end = text.find('\n', begin);
        lines.emplace_back(text.substr(begin, end == std::string_view::npos ? text.size() - begin : end - begin));
        if (end == std::string_view::npos) break;
        begin = end + 1;
    }
    return lines;
}
inline std::vector<std::string> wrap_plain(std::string_view text, std::size_t width) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    for (;;) {
        const auto newline = text.find('\n', begin);
        const auto source = text.substr(begin, newline == std::string_view::npos ? text.size() - begin : newline - begin);
        std::string line;
        std::size_t line_width = 0;
        std::size_t i = 0;
        while (i < source.size()) {
            if (source[i] == ' ' || source[i] == '\t') {
                const std::size_t whitespace_begin = i;
                while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) ++i;
                const auto whitespace = source.substr(whitespace_begin, i - whitespace_begin);
                std::size_t whitespace_width = utf8_width(whitespace);
                std::size_t word_end = i;
                while (word_end < source.size() && source[word_end] != ' ' && source[word_end] != '\t') {
                    const auto cluster = decode_cluster(source, word_end);
                    word_end += cluster.bytes;
                }
                const std::size_t word_width = utf8_width(source.substr(i, word_end - i));
                if (!line.empty() && line_width + whitespace_width + word_width > width) {
                    result.push_back(std::move(line));
                    line.clear(); line_width = 0;
                    continue;
                }
                if (line.empty()) continue;
                line.append(whitespace);
                line_width += whitespace_width;
                continue;
            }
            const auto cluster = decode_cluster(source, i);
            if (!line.empty() && line_width + cluster.width > width) {
                result.push_back(std::move(line));
                line.clear(); line_width = 0;
            }
            line.append(source.substr(i, cluster.bytes));
            line_width += cluster.width;
            i += cluster.bytes;
        }
        result.push_back(std::move(line));
        if (newline == std::string_view::npos) break;
        begin = newline + 1;
    }
    return result;
}
inline std::vector<std::string> wrap_cell(std::string_view text, std::size_t width) {
    if (text.find('[') == std::string_view::npos && text.find('\x1b') == std::string_view::npos &&
        text.find('\\') == std::string_view::npos) return wrap_plain(text, width);
    std::vector<std::string> lines(1);
    std::vector<std::string> open_tags;
    std::size_t line_width = 0;
    auto close_tags = [&] {
        for (auto it = open_tags.rbegin(); it != open_tags.rend(); ++it) lines.back() += "[/" + *it + "]";
    };
    auto reopen_tags = [&] {
        for (const auto& tag : open_tags) lines.back() += "[" + tag + "]";
    };
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            std::size_t end = i + 2;
            while (end < text.size() && (text[end] < '@' || text[end] > '~')) ++end;
            if (end < text.size()) { lines.back().append(text.substr(i, end - i + 1)); i = end + 1; continue; }
        }
        if (text[i] == '\\' && i + 1 < text.size() && (text[i + 1] == '[' || text[i + 1] == ']')) {
            if (line_width == width) { close_tags(); lines.emplace_back(); line_width = 0; reopen_tags(); }
            lines.back().append(text.substr(i, 2)); ++line_width; i += 2; continue;
        }
        if ((text[i] == '[' || text[i] == ']') && i + 1 < text.size() && text[i + 1] == text[i]) {
            if (line_width == width) { close_tags(); lines.emplace_back(); line_width = 0; reopen_tags(); }
            lines.back().append(text.substr(i, 2)); ++line_width; i += 2; continue;
        }
        if (text[i] == '[') {
            const auto end = text.find(']', i + 1);
            if (end != std::string_view::npos) {
                const auto tag = text.substr(i + 1, end - i - 1);
                StyleState probe;
                if (parse_markup_style(tag, probe)) {
                    lines.back().append(text.substr(i, end - i + 1));
                    open_tags.emplace_back(tag);
                    i = end + 1; continue;
                }
                if (!tag.empty() && tag.front() == '/') {
                    const auto closing = tag.substr(1);
                    if (!open_tags.empty() && (closing.empty() || open_tags.back() == closing)) {
                        lines.back().append(text.substr(i, end - i + 1));
                        open_tags.pop_back(); i = end + 1; continue;
                    }
                }
            }
        }
        if (text[i] == '\n') {
            close_tags(); lines.emplace_back(); line_width = 0; reopen_tags(); ++i; continue;
        }
        const auto cluster = decode_cluster(text, i);
        const auto cells = cluster.width;
        if (line_width > 0 && cells > 0 && line_width + cells > width) {
            close_tags(); lines.emplace_back(); line_width = 0; reopen_tags();
        }
        lines.back().append(text.substr(i, cluster.bytes));
        line_width += cells;
        i += cluster.bytes;
    }
    close_tags();
    return lines;
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
    Table& set_line_style(std::string style) { line_style_ = std::move(style); return *this; }
    Table& set_max_width(std::size_t width) noexcept { max_width_ = width; return *this; }
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
        if (max_width_ > columns_.size() * 3 + 1) {
            const std::size_t available = max_width_ - (columns_.size() * 3 + 1);
            while (std::accumulate(widths.begin(), widths.end(), std::size_t{0}) > available) {
                auto widest = std::max_element(widths.begin(), widths.end());
                if (widest == widths.end() || *widest <= 1) break;
                --(*widest);
            }
        }

        std::string output;
        if (!title_.empty()) output += "[bold]" + title_ + "[/bold]\n";
        auto border = [&](std::string_view left, std::string_view join, std::string_view right,
                          std::string_view horizontal, std::string_view style = {}) {
            std::string line = std::string(left);
            for (std::size_t c = 0; c < widths.size(); ++c) {
                line += detail::repeat(horizontal, widths[c] + 2);
                line += (c + 1 == widths.size()) ? std::string(right) : std::string(join);
            }
            const std::string_view selected_style = style.empty() ? std::string_view{border_style_} : style;
            return "[" + std::string(selected_style) + "]" + line + "[/" + std::string(selected_style) + "]\n";
        };
        auto render_row = [&](const std::vector<std::string>& cells, bool header, std::size_t row_index) {
            std::vector<std::vector<std::string>> wrapped(widths.size());
            std::size_t height = 1;
            for (std::size_t c = 0; c < widths.size(); ++c) {
                wrapped[c] = detail::wrap_cell(cells[c], widths[c]);
                height = std::max(height, wrapped[c].size());
            }
            const std::string& row_style = header ? header_style_ : (row_index % 2 == 0 ? even_row_style_ : odd_row_style_);
            for (std::size_t r = 0; r < height; ++r) {
                std::string line = "[" + border_style_ + "]┃[/" + border_style_ + "]";
                for (std::size_t c = 0; c < widths.size(); ++c) {
                    const std::string value = r < wrapped[c].size() ? wrapped[c][r] : std::string{};
                    const std::string cell = detail::pad_cell(value, widths[c], columns_[c].alignment);
                    const std::string content = row_style.empty() ? cell : "[" + row_style + "]" + cell + "[/" + row_style + "]";
                    line += " " + content + " ";
                    line += "[" + border_style_ + "]┃[/" + border_style_ + "]";
                }
                output += line + '\n';
            }
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
            if (show_lines_ && r + 1 < rows_.size()) {
                output += border("┣", "╋", "┫", "┄", line_style_);
            }
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
    std::string line_style_{"bright_black"};
    std::size_t max_width_{0};
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
