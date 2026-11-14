#include <richterm/richterm.hpp>

#include <chrono>
#include <thread>

int main() {
    richterm::Console console;
    richterm::Progress progress(console, 28);
    progress.set_bar_color(richterm::Color::bright_green);
    const auto task = progress.add_task("Compile", 100.0);
    progress.start();
    for (int value = 0; value <= 100; value += 10) {
        progress.update(task, static_cast<double>(value));
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
    progress.stop();
}
