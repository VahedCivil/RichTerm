#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    console.rule("Console");
    console.print("Print, write, println and terminal-aware clear are available.");
    console.write("Working...");
    console.println(" done");
    console.pause(0.1);
}
