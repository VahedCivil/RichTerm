#include <richterm/richterm.hpp>

#include <chrono>

int main() {
    richterm::Console console;
    const auto now = std::chrono::system_clock::now();
    console.print("Date: {}", richterm::format_date(now));
    console.print("Time: {}", richterm::format_time(now));
    console.print("Elapsed: {}", richterm::format_duration(std::chrono::seconds{93784}));
}
