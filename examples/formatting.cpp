#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    console.print("Decimal: {:,.2f}", 1234567.891);
    console.print("Hex: {:#08x}, percentage: {:.1%}", 48879, 0.875);
}
