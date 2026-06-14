#include <fstream>
#include <iostream>

#include "gameboy.hpp"
#include "log.hpp"

int main() {
    try {
        std::ifstream reader("resources/cpu_instrs.gb");

        GB::GameBoy device(construct_cartridge(reader));
        device.run();
    } catch(const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        Log::log<Log::Level::Error>("Fatal exception: {}", e.what());

        return 1;
    }
}