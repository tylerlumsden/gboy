#include <fstream>

#include "gameboy.hpp"

int main() {
    std::ifstream reader("resources/pkmnblue.gb");
    Rom pkmn_data(reader);

    std::ofstream writer("pkmnblue.dat");
    pkmn_data.write_as_hex(writer);
}