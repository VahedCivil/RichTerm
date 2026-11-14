#include <richterm/richterm.hpp>

#include <chrono>
#include <thread>

int main() {
    richterm::Console console;
    richterm::Status status(console, "Preparing report");
    status.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    status.update("Writing output");
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    status.stop("Finished");
}
