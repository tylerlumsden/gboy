#include "gameboy.hpp"
#include "rom.hpp"
#include "data_types.hpp"
#include "log.hpp"

#include <format>
#include <stdexcept>

GameBoy::GameBoy(Rom rom) : rom(rom) {
    auto memory_write = [this](Address addr, Byte data) {
        Log::log<Log::Level::Debug>("Writing to memory address {:#x} with data {}", addr, data);
        if(0x0 <= addr && addr <= 0x3fff) {
            throw std::logic_error(std::format(
                "Attempted to write to ROM address {:#x}\n", addr
            ));
        } 
        
        else {
            this->memory_map[addr - 0x3fff] = data;
            return;
            /*
            throw std::logic_error(std::format(
                "Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
            */
        }
    };

    auto memory_read = [this](Address addr) -> const Byte& {
        Log::log<Log::Level::Debug>("Reading from memory address {:#x}", addr);
        if(0x0 <= addr && addr <= 0x3fff) {
            return this->rom.data[addr];
        } 
        
        else {
            return this->memory_map[addr - 0x3fff];
            /*
            throw std::logic_error(std::format(
                "Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
            */
        }   
    };

    MemoryBus addressable_space{memory_write, memory_read};
    this->processor.addressable_space = addressable_space;
}

void GameBoy::run() {
    this->processor.fetch_decode_execute();
}   