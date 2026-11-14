#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace richterm {

[[nodiscard]] inline std::string format_datetime(
    std::chrono::system_clock::time_point value,
    std::string_view pattern = "%Y-%m-%d %H:%M:%S") {
    const std::time_t time = std::chrono::system_clock::to_time_t(value);
    std::tm local{};
#if defined(_WIN32)
    if (localtime_s(&local, &time) != 0) return {};
#else
    if (localtime_r(&time, &local) == nullptr) return {};
#endif
    std::ostringstream output;
    output << std::put_time(&local, std::string(pattern).c_str());
    return output.str();
}

[[nodiscard]] inline std::string format_date(
    std::chrono::system_clock::time_point value,
    std::string_view pattern = "%Y-%m-%d") {
    return format_datetime(value, pattern);
}

[[nodiscard]] inline std::string format_time(
    std::chrono::system_clock::time_point value,
    std::string_view pattern = "%H:%M:%S") {
    return format_datetime(value, pattern);
}

template <typename Rep, typename Period>
[[nodiscard]] inline std::string format_duration(std::chrono::duration<Rep, Period> value) {
    const long double total_seconds = std::chrono::duration<long double>(value).count();
    if (!std::isfinite(total_seconds) ||
        std::fabs(total_seconds) > static_cast<long double>(std::numeric_limits<std::uint64_t>::max()))
        throw std::overflow_error("Duration is outside the supported formatting range");
    const bool negative = total_seconds < 0;
    const auto magnitude = static_cast<std::uint64_t>(std::fabs(total_seconds));
    const auto hours = magnitude / 3600;
    const auto minutes = (magnitude % 3600) / 60;
    const auto seconds = magnitude % 60;
    std::ostringstream output;
    if (negative) output << '-';
    if (hours > 0) output << hours << ':' << std::setfill('0') << std::setw(2) << minutes;
    else output << minutes;
    output << ':' << std::setfill('0') << std::setw(2) << seconds;
    return output.str();
}

} // namespace richterm
