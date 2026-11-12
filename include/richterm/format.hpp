#pragma once

#include "style.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace richterm {

class format_error : public std::runtime_error {
public:
    explicit format_error(const std::string& message) : std::runtime_error(message) {}
};

namespace detail {
struct FormatSpec {
    char fill{' '};
    char align{'\0'}; // <, >, ^, =
    char sign{'-'};    // -, +, space
    bool alternate{false};
    bool zero{false};
    char grouping{'\0'};
    int width{0};
    int precision{-1};
    char type{'\0'};
};

inline FormatSpec parse_spec(std::string_view spec) {
    FormatSpec out;
    std::size_t i = 0;
    if (spec.size() >= 2 && (spec[1] == '<' || spec[1] == '>' || spec[1] == '^' || spec[1] == '=')) {
        out.fill = spec[0]; out.align = spec[1]; i = 2;
    } else if (i < spec.size() && (spec[i] == '<' || spec[i] == '>' || spec[i] == '^' || spec[i] == '=')) {
        out.align = spec[i++];
    }
    if (i < spec.size() && (spec[i] == '+' || spec[i] == '-' || spec[i] == ' ')) out.sign = spec[i++];
    if (i < spec.size() && spec[i] == '#') { out.alternate = true; ++i; }
    if (i < spec.size() && spec[i] == '0') { out.zero = true; ++i; }
    while (i < spec.size() && spec[i] >= '0' && spec[i] <= '9') {
        if (out.width > 100000) throw format_error("Format width exceeds 1,000,000");
        out.width = out.width * 10 + (spec[i++] - '0');
    }
    if (i < spec.size() && (spec[i] == ',' || spec[i] == '_')) out.grouping = spec[i++];
    if (i < spec.size() && spec[i] == '.') {
        ++i;
        if (i == spec.size() || spec[i] < '0' || spec[i] > '9') throw format_error("Precision requires decimal digits");
        out.precision = 0;
        while (i < spec.size() && spec[i] >= '0' && spec[i] <= '9') {
            if (out.precision > 10000) throw format_error("Format precision exceeds 100,000");
            out.precision = out.precision * 10 + (spec[i++] - '0');
        }
    }
    if (i < spec.size()) out.type = spec[i++];
    if (i != spec.size()) throw format_error("Invalid format specification: " + std::string(spec));
    if (out.type && std::string_view("dxXobBcfsFeEgGaA%").find(out.type) == std::string_view::npos)
        throw format_error("Unsupported format type: " + std::string(1, out.type));
    if (out.zero) { if (out.align == '\0') out.align = '='; out.fill = '0'; }
    return out;
}

inline std::string apply_width(std::string value, const FormatSpec& spec, bool numeric = false) {
    if (static_cast<int>(value.size()) >= spec.width) return value;
    const int count = spec.width - static_cast<int>(value.size());
    char align = spec.align ? spec.align : (numeric ? '>' : '<');
    if (align == '=') {
        std::size_t prefix = 0;
        if (!value.empty() && (value[0] == '+' || value[0] == '-' || value[0] == ' ')) prefix = 1;
        if (value.compare(prefix, 2, "0x") == 0 || value.compare(prefix, 2, "0X") == 0 ||
            value.compare(prefix, 2, "0b") == 0 || value.compare(prefix, 2, "0B") == 0 ||
            value.compare(prefix, 2, "0o") == 0) prefix += 2;
        return value.substr(0, prefix) + std::string(static_cast<std::size_t>(count), spec.fill) + value.substr(prefix);
    }
    if (align == '<') return value + std::string(static_cast<std::size_t>(count), spec.fill);
    if (align == '^') {
        const int left = count / 2;
        return std::string(static_cast<std::size_t>(left), spec.fill) + value +
               std::string(static_cast<std::size_t>(count - left), spec.fill);
    }
    return std::string(static_cast<std::size_t>(count), spec.fill) + value;
}

inline std::string add_grouping(std::string value, char separator = ',') {
    std::size_t start = 0;
    if (!value.empty() && (value[0] == '+' || value[0] == '-' || value[0] == ' ')) start = 1;
    if (value.compare(start, 2, "0x") == 0 || value.compare(start, 2, "0X") == 0 ||
        value.compare(start, 2, "0b") == 0 || value.compare(start, 2, "0B") == 0 ||
        value.compare(start, 2, "0o") == 0) start += 2;
    const auto dot = value.find('.', start);
    const std::size_t end = dot == std::string::npos ? value.size() : dot;
    for (std::size_t pos = end; pos > start + 3; ) { pos -= 3; value.insert(pos, 1, separator); }
    return value;
}

template <typename T, bool = std::is_enum_v<T>> struct enum_or_self { using type = T; };
template <typename T> struct enum_or_self<T, true> { using type = std::underlying_type_t<T>; };
template <typename T> struct is_style_token : std::false_type {};
template <> struct is_style_token<Color> : std::true_type {};
template <> struct is_style_token<Style> : std::true_type {};
template <> struct is_style_token<BackgroundColor> : std::true_type {};
template <> struct is_style_token<TextStyle> : std::true_type {};
template <typename T> inline constexpr bool is_style_token_v = is_style_token<std::remove_cvref_t<T>>::value;

template <typename T>
std::string format_value(const T& value, const FormatSpec& spec) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, std::string_view>) {
        if (spec.type != '\0' && spec.type != 's') throw format_error("String used with non-string format type");
        std::string result(value);
        if (spec.precision >= 0 && static_cast<int>(result.size()) > spec.precision) result.resize(static_cast<std::size_t>(spec.precision));
        return apply_width(std::move(result), spec, false);
    } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
        if (value == nullptr) throw format_error("Null C-string argument");
        return format_value(std::string_view{value}, spec);
    } else if constexpr (std::is_array_v<U> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<U>>, char>) {
        return format_value(std::string_view{value}, spec);
    } else if constexpr (std::is_same_v<U, char>) {
        if (spec.type == 'c' || spec.type == '\0' || spec.type == 's') return apply_width(std::string(1, value), spec, false);
        return format_value(static_cast<int>(static_cast<unsigned char>(value)), spec);
    } else if constexpr (std::is_same_v<U, bool>) {
        if (spec.type == 'd' || spec.type == 'x' || spec.type == 'X' || spec.type == 'o' || spec.type == 'b' || spec.type == 'B')
            return format_value(static_cast<unsigned int>(value), spec);
        if (spec.type != '\0' && spec.type != 's') throw format_error("Boolean used with incompatible format type");
        return apply_width(value ? "true" : "false", spec, false);
    } else if constexpr (std::is_integral_v<U> || std::is_enum_v<U>) {
        using Raw = typename enum_or_self<U>::type;
        const Raw number = static_cast<Raw>(value);
        char type = spec.type ? spec.type : 'd';
        if (type == 'c') {
            if (spec.precision >= 0) throw format_error("Character format does not accept precision");
            return apply_width(std::string(1, static_cast<char>(number)), spec, false);
        }
        if (type != 'd' && type != 'x' && type != 'X' && type != 'o' && type != 'b' && type != 'B')
            throw format_error("Integer used with incompatible format type");
        if (spec.precision >= 0) throw format_error("Integer precision is not supported");
        const bool negative = std::is_signed_v<Raw> && number < 0;
        using Unsigned = std::make_unsigned_t<Raw>;
        Unsigned magnitude = negative ? Unsigned(0) - static_cast<Unsigned>(number) : static_cast<Unsigned>(number);
        int base = (type == 'x' || type == 'X') ? 16 : ((type == 'o') ? 8 : ((type == 'b' || type == 'B') ? 2 : 10));
        const char* digits = (type == 'X' || type == 'B') ? "0123456789ABCDEF" : "0123456789abcdef";
        std::string body;
        do { body.insert(body.begin(), digits[static_cast<unsigned>(magnitude % static_cast<Unsigned>(base))]); magnitude /= static_cast<Unsigned>(base); } while (magnitude != 0);
        std::string result;
        if (negative) result = "-";
        else if (spec.sign == '+') result = "+";
        else if (spec.sign == ' ') result = " ";
        if (spec.alternate) {
            if (base == 16) result += (type == 'X' ? "0X" : "0x");
            else if (base == 8) result += "0o";
            else if (base == 2) result += (type == 'B' ? "0B" : "0b");
        }
        result += body;
        if (spec.grouping != '\0' && base == 10) result = add_grouping(std::move(result), spec.grouping);
        return apply_width(std::move(result), spec, true);
    } else if constexpr (std::is_floating_point_v<U>) {
        char type = spec.type ? spec.type : 'g';
        if (type != 'f' && type != 'F' && type != 'e' && type != 'E' && type != 'g' && type != 'G' && type != 'a' && type != 'A' && type != '%')
            throw format_error("Floating-point value used with incompatible format type");
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        const int precision = spec.precision >= 0 ? spec.precision : 6;
        if (type == 'f' || type == 'F' || type == '%') stream << std::fixed;
        else if (type == 'e' || type == 'E') stream << std::scientific;
        else if (type == 'a' || type == 'A') stream << std::hexfloat;
        else stream << std::defaultfloat;
        if (type == 'F' || type == 'E' || type == 'G' || type == 'A') stream << std::uppercase;
        if (spec.alternate) stream << std::showpoint;
        stream << std::setprecision(precision);
        long double number = static_cast<long double>(value);
        if (type == '%') number *= 100.0L;
        stream << number;
        std::string result = stream.str();
        if (type == '%' ) result += '%';
        if (!result.empty() && result[0] != '-' && result[0] != '+') {
            if (spec.sign == '+') result.insert(result.begin(), '+');
            else if (spec.sign == ' ') result.insert(result.begin(), ' ');
        }
        if (spec.grouping != '\0') result = add_grouping(std::move(result), spec.grouping);
        return apply_width(std::move(result), spec, true);
    } else {
        if (spec.type != '\0' && spec.type != 's') throw format_error("Unsupported type for format argument");
        std::ostringstream stream; stream << value;
        return apply_width(stream.str(), spec, false);
    }
}

template <std::size_t I = 0, typename Tuple>
std::string format_tuple_arg(const Tuple& tuple, std::size_t index, const FormatSpec& spec) {
    if constexpr (I < std::tuple_size_v<Tuple>) {
        if (I == index) {
            using T = std::remove_cvref_t<decltype(std::get<I>(tuple))>;
            if constexpr (is_style_token_v<T>) return {};
            else return format_value(std::get<I>(tuple), spec);
        }
        return format_tuple_arg<I + 1>(tuple, index, spec);
    } else {
        throw format_error("Not enough arguments for format string");
    }
}

template <typename Tuple>
std::string format_tuple(std::string_view input, const Tuple& args) {
    std::string output;
    std::size_t arg_index = 0;
    for (std::size_t i = 0; i < input.size();) {
        if (input[i] == '{') {
            if (i + 1 < input.size() && input[i + 1] == '{') { output += '{'; i += 2; continue; }
            const auto end = input.find('}', i + 1);
            if (end == std::string_view::npos) throw format_error("Unmatched '{' in format string");
            const auto field = input.substr(i + 1, end - i - 1);
            if (!field.empty() && field.front() != ':') throw format_error("Only automatic replacement fields are supported");
            const auto spec_text = field.empty() ? std::string_view{} : field.substr(1);
            auto spec = parse_spec(spec_text);
            output += format_tuple_arg(args, arg_index++, spec);
            i = end + 1;
        } else if (input[i] == '}') {
            if (i + 1 < input.size() && input[i + 1] == '}') { output += '}'; i += 2; }
            else throw format_error("Unmatched '}' in format string");
        } else output += input[i++];
    }
    if (arg_index != std::tuple_size_v<Tuple>) throw format_error("Too many arguments for format string");
    return output;
}

struct FormatSegment { std::string text; StyleState style; };

template <std::size_t I = 0, typename Tuple>
void format_tuple_arg_segments(const Tuple& tuple, std::size_t index, const FormatSpec& spec,
                               StyleState& active, std::vector<FormatSegment>& output) {
    if constexpr (I < std::tuple_size_v<Tuple>) {
        if (I == index) {
            const auto& value = std::get<I>(tuple);
            using T = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Color> || std::is_same_v<T, Style> ||
                          std::is_same_v<T, BackgroundColor> || std::is_same_v<T, TextStyle>) {
                if (spec.type || spec.precision >= 0 || spec.width != 0)
                    throw format_error("Style tokens do not accept format specifications");
                apply_token(active, value);
            } else {
                output.push_back({format_value(value, spec), active});
            }
            return;
        }
        format_tuple_arg_segments<I + 1>(tuple, index, spec, active, output);
    } else {
        throw format_error("Not enough arguments for format string");
    }
}

template <typename Tuple>
std::vector<FormatSegment> format_tuple_segments(std::string_view input, const Tuple& args) {
    std::vector<FormatSegment> output;
    std::string literal;
    StyleState active;
    std::size_t arg_index = 0;
    auto flush = [&] {
        if (!literal.empty()) { output.push_back({std::move(literal), active}); literal.clear(); }
    };
    for (std::size_t i = 0; i < input.size();) {
        if (input[i] == '{') {
            if (i + 1 < input.size() && input[i + 1] == '{') { literal += '{'; i += 2; continue; }
            const auto end = input.find('}', i + 1);
            if (end == std::string_view::npos) throw format_error("Unmatched '{' in format string");
            const auto field = input.substr(i + 1, end - i - 1);
            if (!field.empty() && field.front() != ':') throw format_error("Only automatic replacement fields are supported");
            const auto spec_text = field.empty() ? std::string_view{} : field.substr(1);
            const auto spec = parse_spec(spec_text);
            flush();
            format_tuple_arg_segments(args, arg_index++, spec, active, output);
            i = end + 1;
        } else if (input[i] == '}') {
            if (i + 1 < input.size() && input[i + 1] == '}') { literal += '}'; i += 2; }
            else throw format_error("Unmatched '}' in format string");
        } else literal += input[i++];
    }
    flush();
    if (arg_index != std::tuple_size_v<Tuple>) throw format_error("Too many arguments for format string");
    return output;
}
} // namespace detail

template <typename... Args>
[[nodiscard]] std::vector<detail::FormatSegment> format_segments(std::string_view format_string, Args&&... args) {
    auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
    return detail::format_tuple_segments(format_string, tuple);
}

struct FormatFunction {
    template <typename... Args>
    [[nodiscard]] std::string operator()(std::string_view format_string, Args&&... args) const {
        auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        return detail::format_tuple(format_string, tuple);
    }
};

// A function object deliberately avoids ADL collisions with std::format when
// arguments include std::string or other standard-library types.
inline constexpr FormatFunction format{};

} // namespace richterm
