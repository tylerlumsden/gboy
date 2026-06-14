#include "gameboy.hpp"
#include "data_types.hpp"
#include "log.hpp"

#include <format>
#include <stdexcept>

namespace GB {

GameBoy::GameBoy(Cartridge cartridge) : cart(cartridge) {}

Byte read(GameBoy& gb, Address addr) {
    Log::log<Log::Level::Debug>("Reading from memory address {:#x}", addr);
    if(0x0 <= addr && addr <= 0x7fff) {
        return gb.cart.read(addr);
    }
    else if(0xa000 <= addr && addr <= 0xbfff) {
        return gb.cart.read(addr);
    }
    else {
        throw std::logic_error(std::format(
            "Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
    }
}

void write(GameBoy& gb, Address addr, Byte data) {
    Log::log<Log::Level::Debug>("Writing to memory address {:#x} with data {}", addr, data);
    if(0x0 <= addr && addr <= 0x7fff) {
        gb.cart.write(addr, data);
    }
    else if(0xa000 <= addr && addr <= 0xbfff) {
        gb.cart.write(addr, data);
    }
    else {
        throw std::logic_error(std::format(
            "Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
    }
}

void GameBoy::run() {
    SM83::fetch_decode_execute(*this);
}

}
