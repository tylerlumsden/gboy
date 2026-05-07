#include <fstream>

#include "gameboy.hpp"

int main() {
    std::ifstream reader("resources/pkmnblue.gb");
    Rom pkmn_data(reader);

    GameBoy device{pkmn_data};
    device.run();
}