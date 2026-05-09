#include "gameboy.hpp"
#include "rom.hpp"
#include "data_types.hpp"

#include <format>
#include <functional>
#include <stdexcept>
#include <fstream>

// TODO: need to differentiate between reads/writes
// For example maybe implement separate read/write functions
// Perhaps don't even make these function a member, just create it in the constructor
Byte& GameBoy::memory_map(Address addr) {
    if(0x0 <= addr && addr <= 0x3FFF) {
        return this->rom.data[addr];
    } else {
        throw std::logic_error(std::format(
            "Attempted to access address {}. This address is either unimplemented or out of range.\n", addr
        ));
    }   
}

GameBoy::GameBoy(Rom rom) : rom(rom) {
    MemoryBus addressable_space([this](Address addr) -> Byte& {
        return this->memory_map(addr);
    });

    this->processor.addressable_space = addressable_space;
}

void GameBoy::run() {
    std::ofstream debug_output("resources/pkmnblue.dat");
    rom.write_as_hex(debug_output);
    this->processor.fetch_decode_execute();
}   