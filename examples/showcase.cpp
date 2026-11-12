#include <richterm/richterm.hpp>

#include <iostream>
#include <chrono>
#include <sstream>
#include <thread>

int main() {
    using namespace richterm;

    Console console;
    console.print("[bold cyan]RichTerm[/bold cyan] — terminal output for modern C++");
    console.print("BBCode-style markup: [bold cyan]Will[/bold cyan] [u]is[/u] [i]a way[/i].");

    console.print("Formatting: integer={:08d}, hex={:#010x}, grouped={:,}, pi={:.12f}",
                   314, 48879, 123456789, 3.141592653589793);
    console.print("Alignment: |{:<12}| |{:^12}| |{:>12}|", "left", "center", "right");

    console.print("{}Colorma-style tokens{} can change colors inside a format string.",
                   Color::bright_green, Style::reset);
    console.print("Combined style: {}warning{} and background {} pass {}.",
                   Style::bold + Color::bright_yellow,
                   Style::reset,
                   Color::black + on(Color::bright_green) + Style::bold,
                   Style::reset);

    Table table("Build summary");
    table.add_column("Target", Alignment::left)
         .add_column("Status", Alignment::center)
         .add_column("Time (s)", Alignment::right)
         .add_row("RichTerm", "[bold green]PASS[/bold green]", 0.042)
         .add_row("Colorma compatibility", "[green]PASS[/green]", 0.018)
         .add_row("Install package", "[yellow]READY[/yellow]", 0.011)
         .set_caption("Example table with aligned numeric data");
    console.print(table.render());

    Progress progress(console, 28, true, 15.0);
    const auto compile = progress.add_task("Compile targets", 100.0, 0.0);
    const auto tests = progress.add_task("Run tests", 12.0, 0.0);
    progress.start();
    for (int step = 1; step <= 100; ++step) {
        progress.update(compile, static_cast<double>(step));
        if (step % 8 == 0 && step <= 96) progress.advance(tests, 1.0);
        std::this_thread::sleep_for(std::chrono::milliseconds{22});
    }
    progress.update(tests, 12.0);
    progress.stop();

    Status status(console, "Preparing release package", 12.0);
    status.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{650});
    status.update("Checking CMake install layout");
    std::this_thread::sleep_for(std::chrono::milliseconds{650});
    status.stop("Release package ready");

    return 0;
}
