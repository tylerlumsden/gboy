#include <fstream>

#include "gameboy.hpp"

int main() {
    std::ifstream reader("resources/cpu_instrs.gb");
    Rom rom(reader);

    GameBoy device(rom);
    device.run();
}