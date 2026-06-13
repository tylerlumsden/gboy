#include "gameboy.hpp"
#include "data_types.hpp"
#include "log.hpp"

#include <format>
#include <stdexcept>

GameBoy::GameBoy(Cartridge cartridge) : cart(cartridge) {
    auto memory_write = [this](Address addr, Byte data) {
        Log::log<Log::Level::Debug>("Writing to memory address {:#x} with data {}", addr, data);
        if(0x0 <= addr && addr <= 0x7fff) {
            this->cart.write(addr, data);
        }
        else if(0xa000 <= addr && addr <= 0xbfff) {
            this->cart.write(addr, data);
        }
        else {
            throw std::logic_error(std::format(
                "Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    };

    auto memory_read = [this](Address addr) -> Byte {
        Log::log<Log::Level::Debug>("Reading from memory address {:#x}", addr);
        if(0x0 <= addr && addr <= 0x7fff) {
            return this->cart.read(addr);
        } 
        else if(0xa000 <= addr && addr <= 0xbfff) {
            return this->cart.read(addr);
        }
        else {
            throw std::logic_error(std::format(
                "Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }   
    };

    MemoryBus addressable_space{memory_write, memory_read};
    this->processor.addressable_space = addressable_space;
}

void GameBoy::run() {
    this->processor.fetch_decode_execute();
}   