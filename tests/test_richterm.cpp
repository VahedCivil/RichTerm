#include <richterm/richterm.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template <typename Function>
void require_throws(Function&& function, const char* message) {
    bool thrown = false;
    try { function(); } catch (const std::exception&) { thrown = true; }
    require(thrown, message);
}
}

int main() {
    using namespace richterm;

    // Format strings: braces, width, alignment, numeric bases, signs and precision.
    require(format("{} {} {{ok}}", "RichTerm", 17) == "RichTerm 17 {ok}", "Basic formatting failed");
    require(format("{:#08x}", 48879) == "0x00beef", "Alternate hexadecimal or zero padding failed");
    require(format("{:+d} {:,}", 42, 123456789) == "+42 123,456,789", "Sign or digit grouping failed");
    require(format("{:.3f}", 3.141592653589793) == "3.142", "Fixed floating-point precision failed");
    require(format("{:.2e}", 1234.0) == "1.23e+03", "Scientific formatting failed");
    require(format("|{:<6}|{:^6}|{:>6}|", "a", "b", "c") == "|a     |  b   |     c|", "Text alignment failed");
    require(format("{:*>6}", "OK") == "****OK", "Custom fill character failed");
    require(format("{:.3s}", "console") == "con", "String precision failed");
    const std::string owned_text = "owned string";
    require(format("|{:>14s}|", owned_text) == "|  owned string|", "std::string formatting failed");
    require(format("{:_}", 1234567) == "1_234_567", "Underscore digit grouping failed");
    require(format("{:X} {:#b}", 255, 10) == "FF 0b1010", "Uppercase hexadecimal or binary format failed");
    require(format("{:b} {:#o}", 10, 8) == "1010 0o10", "Binary or octal formatting failed");
    require(format("{:.1%}", 0.125) == "12.5%", "Percentage format failed");
    require_throws([] { (void)format("{"); }, "Unmatched opening brace must throw");
    require_throws([] { (void)format("}"); }, "Unmatched closing brace must throw");
    require_throws([] { (void)format("{}", 1, 2); }, "Extra arguments must throw");
    require_throws([] { (void)format("{} {}"); }, "Missing arguments must throw");
    require_throws([] { (void)format("{:d}", "wrong"); }, "Incompatible type must throw");
    const char* null_text = nullptr;
    require_throws([&] { (void)format("{}", null_text); }, "Null C string must throw");

    // Console output and BBCode-style tags. Use forced color to inspect exact escape sequences.
    std::ostringstream sink;
    Console console(sink, ConsoleOptions{ColorMode::always, 80, true});
    console.print("Where there is a [bold cyan]Will[/bold cyan] there [u]is[/u] a [i]way[/i].");
    const auto markup = sink.str();
    require(markup.find("\x1b[36;1mWill\x1b[0m") != std::string::npos, "Bold cyan markup failed");
    require(markup.find("\x1b[4mis\x1b[0m") != std::string::npos, "Underline markup failed");
    require(markup.find("\x1b[3mway\x1b[0m") != std::string::npos, "Italic markup failed");
    sink.str({}); sink.clear();
    console.print("{{literal bracket}} [red]red[/red]");
    require(sink.str().find("{literal bracket}") != std::string::npos, "Escaped markup bracket failed");
    require(sink.str().find("\x1b[31mred\x1b[0m") != std::string::npos, "Basic color markup failed");

    // Colorma-style formatting tokens.
    sink.str({}); sink.clear();
    console.print("{}green {}red{}", Color::green, Color::red, Style::reset);
    require(sink.str().find("\x1b[32mgreen \x1b[0m") != std::string::npos, "Foreground token failed");
    require(sink.str().find("\x1b[31mred\x1b[0m") != std::string::npos, "Color switch token failed");
    sink.str({}); sink.clear();
    console.print("{}OK{}", Color::black + on(Color::bright_green) + Style::bold, Style::reset);
    require(sink.str().find("\x1b[30;102;1mOK\x1b[0m") != std::string::npos, "Combined style token failed");
    sink.str({}); sink.clear();
    require_throws([&] { console.print("prefix {}", 7, 8); }, "Extra Console arguments must throw");
    require(sink.str().empty(), "Format errors must not partially write to Console");

    // Automatic color mode remains plain when writing to an in-memory stream.
    std::ostringstream plain;
    Console plain_console(plain, ConsoleOptions{ColorMode::automatic, 80, true});
    plain_console.print("[red]plain[/red]");
    require(plain.str() == "plain\n", "Automatic mode must not color redirected streams");
    const auto automatic_colorized = colorize("plain", Color::red, ColorMode::automatic);
    require(automatic_colorized == "plain", "Standalone automatic colorize must respect redirected stdout");
    const auto forced_colorized = colorize("white", Color::bright_white, Color::red,
                                            {Style::bold, Style::underline}, ColorMode::always);
    require(forced_colorized.find("\x1b[") != std::string::npos &&
            forced_colorized.find("white\x1b[0m") != std::string::npos,
            "Foreground/background colorize helper failed");

    // Table layout and consistency checks.
    Table table("Numbers");
    table.add_column("Name").add_column("Value", Alignment::right)
         .add_row("pi", "3.14159").add_row("integer", "42");
    const auto table_text = table.render();
    require(table_text.find("Numbers") != std::string::npos, "Table title missing");
    require(table_text.find("┏") != std::string::npos && table_text.find("┗") != std::string::npos, "Table frame missing");
    sink.str({}); sink.clear();
    console.print(table);
    require(sink.str().find("Numbers") != std::string::npos, "Console::print(Table) overload failed");
    sink.str({}); sink.clear();
    console.rule("Section");
    require(sink.str().find("Section") != std::string::npos, "Console rule rendering failed");
    bool table_error = false;
    try { table.add_row("only one cell"); } catch (const std::invalid_argument&) { table_error = true; }
    require(table_error, "Mismatched table row must throw");

    // Progress and Status animate on TTYs, and use predictable snapshots in redirected streams.
    std::ostringstream progress_sink;
    Console progress_console(progress_sink, ConsoleOptions{ColorMode::never, 80, true});
    Progress progress(progress_console, 10, true, 30.0);
    const auto id = progress.add_task("Download", 10.0, 2.0);
    progress.advance(id, 3.0);
    require(progress.tasks().at(id).completed == 5.0, "Progress advance failed");
    require(progress.render().find("50.0%") != std::string::npos, "Progress percentage render failed");
    progress.start();
    require(progress.active(), "Progress start failed");
    progress.advance(id, 1.0);
    progress.refresh();
    require(progress_sink.str().find("60.0%") != std::string::npos,
            "Progress refresh must show the latest redirected snapshot");
    require_throws([&] { progress.advance(100); }, "Unknown progress task must throw");
    require_throws([&] { (void)progress.add_task("invalid", std::numeric_limits<double>::infinity()); },
                   "Non-finite progress totals must throw");
    require_throws([&] { progress.update(id, std::numeric_limits<double>::quiet_NaN()); },
                   "Non-finite progress completion must throw");
    const auto empty_task = progress.add_task("No work required", 0.0);
    (void)empty_task;
    require(progress.render().find("100.0%") != std::string::npos,
            "Zero-total progress tasks should render as complete");
    progress.stop();
    require(!progress.active(), "Progress stop failed");

    Status status(progress_console, "Working", 30.0);
    status.start();
    require(status.active(), "Status start failed");
    status.update("Almost done");
    status.stop("Done");
    require(!status.active(), "Status stop failed");
    require(progress_sink.str().find("Almost done") != std::string::npos, "Status update missing");
    require(progress_sink.str().find("Done") != std::string::npos, "Status final message missing");
    require_throws([&] { Progress invalid_rate(progress_console, 10, true, 0.0); },
                   "Progress refresh rate must be validated");
    require_throws([&] { Status invalid_status_rate(progress_console, "x", 1001.0); },
                   "Status refresh rate must be validated");

    std::cout << "All RichTerm tests passed.\n";
    return 0;
}
