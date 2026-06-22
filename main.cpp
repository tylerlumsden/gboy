#include <fstream>
#include <iostream>

#include "gameboy.hpp"
#include "log.hpp"

int main(int argc, char ** argv) {
    try {
        if(argc < 2) {
            throw std::invalid_argument("Requires path to gameboy rom.");
        }

        Cartridge game = construct_cartridge(std::ifstream(argv[1]));
        
        write_as_hex(game.rom.data, std::ofstream("resources/test.dat"));

        GB::GameBoy device(game);
        device.run();
    } catch(const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        Log::log<Log::Level::Error>("Fatal exception: {}", e.what());

        return 1;
    }
}