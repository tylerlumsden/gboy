#include "gameboy.hpp"
#include "data_types.hpp"
#include "log.hpp"

#include <format>
#include <stdexcept>

GameBoy::GameBoy(Cartridge cartridge) : cart(cartridge) {
    auto memory_write = [this](Address addr, Byte data) {
        Log::log<Log::Level::Debug>("Writing to memory address {:#x} with data {}", addr, data);
        if(0x0 <= addr && addr <= 0x1fff) {
            //this->cart.impl.ram.ram_toggle = ((data & 0x0a) == 0x0a);
        }
        else if(0x2000 <= addr && addr <= 0x3fff) {
            this->cart.impl.rom.set_bank_id(data);
        }
        else if(0x4000 <= addr && addr <= 0x7fff) {
            throw std::logic_error(std::format(
                "Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
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
            return this->cart.impl.rom.data[addr];
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