#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace richterm::detail {

struct Utf8CodePoint {
    char32_t value{};
    std::size_t bytes{1};
};

[[nodiscard]] inline Utf8CodePoint decode_utf8(std::string_view text, std::size_t offset) noexcept {
    const auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80u) return {first, 1};
    std::size_t count = 0;
    char32_t value = 0;
    if ((first & 0xE0u) == 0xC0u) { count = 2; value = first & 0x1Fu; }
    else if ((first & 0xF0u) == 0xE0u) { count = 3; value = first & 0x0Fu; }
    else if ((first & 0xF8u) == 0xF0u) { count = 4; value = first & 0x07u; }
    else return {0xFFFD, 1};
    if (offset + count > text.size()) return {0xFFFD, 1};
    for (std::size_t i = 1; i < count; ++i) {
        const auto byte = static_cast<unsigned char>(text[offset + i]);
        if ((byte & 0xC0u) != 0x80u) return {0xFFFD, 1};
        value = static_cast<char32_t>((value << 6u) | (byte & 0x3Fu));
    }
    if ((count == 2 && value < 0x80) || (count == 3 && value < 0x800) ||
        (count == 4 && value < 0x10000) || value > 0x10FFFF ||
        (value >= 0xD800 && value <= 0xDFFF)) return {0xFFFD, 1};
    return {value, count};
}

[[nodiscard]] inline bool in_range(char32_t value, char32_t first, char32_t last) noexcept {
    return value >= first && value <= last;
}

[[nodiscard]] inline int codepoint_width(char32_t value) noexcept {
    if (value == 0) return 0;
    if (value < 32 || in_range(value, 0x7F, 0x9F)) return 0;
    // Combining marks, variation selectors, and joiners do not occupy a cell.
    if (in_range(value, 0x1F3FB, 0x1F3FF) ||
        in_range(value, 0x0300, 0x036F) || in_range(value, 0x0483, 0x0489) ||
        in_range(value, 0x0591, 0x05BD) || value == 0x05BF ||
        in_range(value, 0x05C1, 0x05C2) || in_range(value, 0x0610, 0x061A) ||
        in_range(value, 0x064B, 0x065F) || in_range(value, 0x06D6, 0x06ED) ||
        in_range(value, 0x0711, 0x0711) || in_range(value, 0x0730, 0x074A) ||
        in_range(value, 0x07A6, 0x07B0) || in_range(value, 0x0816, 0x082D) ||
        in_range(value, 0x08D3, 0x0902) || in_range(value, 0x093A, 0x093C) ||
        in_range(value, 0x0941, 0x0948) || in_range(value, 0x0951, 0x0957) ||
        in_range(value, 0x1AB0, 0x1AFF) || in_range(value, 0x1DC0, 0x1DFF) ||
        in_range(value, 0x20D0, 0x20FF) || in_range(value, 0xFE00, 0xFE0F) ||
        in_range(value, 0xFE20, 0xFE2F) || in_range(value, 0xE0100, 0xE01EF) ||
        value == 0x200D) return 0;
    // East Asian wide/full-width characters and common emoji ranges.
    if (in_range(value, 0x1100, 0x115F) || value == 0x2329 || value == 0x232A ||
        in_range(value, 0x2E80, 0xA4CF) || in_range(value, 0xAC00, 0xD7A3) ||
        in_range(value, 0xF900, 0xFAFF) || in_range(value, 0xFE10, 0xFE19) ||
        in_range(value, 0xFE30, 0xFE6F) || in_range(value, 0xFF00, 0xFF60) ||
        in_range(value, 0xFFE0, 0xFFE6) || in_range(value, 0x1F300, 0x1FAFF) ||
        in_range(value, 0x20000, 0x3FFFD)) return 2;
    return 1;
}

struct Utf8Cluster {
    std::size_t bytes{1};
    std::size_t width{1};
};

[[nodiscard]] inline Utf8Cluster decode_cluster(std::string_view text, std::size_t offset) noexcept {
    const auto first = decode_utf8(text, offset);
    Utf8Cluster cluster{first.bytes, static_cast<std::size_t>(codepoint_width(first.value))};
    const bool regional = in_range(first.value, 0x1F1E6, 0x1F1FF);
    bool joined = false;
    std::size_t regional_count = regional ? 1 : 0;
    while (offset + cluster.bytes < text.size()) {
        const auto next = decode_utf8(text, offset + cluster.bytes);
        const bool modifier = in_range(next.value, 0x1F3FB, 0x1F3FF);
        const bool combining = codepoint_width(next.value) == 0;
        if (combining || modifier) {
            cluster.bytes += next.bytes;
            if (next.value == 0x20E3 || next.value == 0xFE0F) cluster.width = 2;
            if (next.value == 0x200D) joined = true;
            continue;
        }
        if (joined) {
            cluster.bytes += next.bytes;
            cluster.width = std::max(cluster.width, static_cast<std::size_t>(codepoint_width(next.value)));
            joined = false;
            continue;
        }
        if (regional && regional_count == 1 && in_range(next.value, 0x1F1E6, 0x1F1FF)) {
            cluster.bytes += next.bytes;
            cluster.width = 2;
            ++regional_count;
            continue;
        }
        break;
    }
    return cluster;
}

[[nodiscard]] inline std::size_t utf8_width(std::string_view text) noexcept {
    std::size_t width = 0;
    for (std::size_t i = 0; i < text.size();) {
        const auto cluster = decode_cluster(text, i);
        width += cluster.width;
        i += cluster.bytes;
    }
    return width;
}

[[nodiscard]] inline std::string wrap_utf8(std::string_view text, std::size_t width) {
    if (width == 0) return std::string{text};
    std::string result;
    std::size_t line_width = 0;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '\n') { result += '\n'; line_width = 0; ++i; continue; }
        const auto cluster = decode_cluster(text, i);
        if (line_width > 0 && cluster.width > 0 && line_width + cluster.width > width) {
            result += '\n';
            line_width = 0;
        }
        result.append(text.substr(i, cluster.bytes));
        line_width += cluster.width;
        i += cluster.bytes;
    }
    return result;
}

} // namespace richterm::detail
