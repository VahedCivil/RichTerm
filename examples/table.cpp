#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    richterm::Table table("Build matrix");
    table.add_column("Platform").add_column("Result", richterm::Alignment::center)
         .add_row("Linux x86_64", "[green]PASS[/green]")
         .add_row("Windows x86_64", "Pending visual check")
         .set_max_width(32).set_line_style("yellow").show_lines();
    console.print(table);
}
