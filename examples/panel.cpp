#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    richterm::Panel panel("Panel content wraps to fit the selected width.", "Build summary");
    panel.set_width(38).set_padding(1).set_border_style("bright_blue");
    console.print(panel);
}
