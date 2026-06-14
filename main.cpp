#include <fstream>
#include <iostream>

#include "gameboy.hpp"
#include "log.hpp"

int main() {
    try {
        Cartridge game = construct_cartridge(std::ifstream("resources/cpu_instrs.gb"));
        
        write_as_hex(game.rom.data, std::ofstream("resources/test.dat"));

        GB::GameBoy device(game);
        device.run();
    } catch(const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        Log::log<Log::Level::Error>("Fatal exception: {}", e.what());

        return 1;
    }
}