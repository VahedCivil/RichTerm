#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    console.print("[bold cyan]Nested[/] styles and [yellow on blue]backgrounds[/yellow on blue].");
    console.print(richterm::Console::escape_markup("[this] is literal markup text"));
}
