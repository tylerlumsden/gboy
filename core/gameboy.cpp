#include "gameboy.hpp"
#include "rom.hpp"
#include "data_types.hpp"

#include <format>
#include <functional>
#include <stdexcept>

GameBoy::GameBoy(Rom rom) : rom(rom) {
    auto memory_write = [this](Address addr, Byte data) {
        if(0x0 <= addr && addr <= 0x3fff) {
            throw std::logic_error(std::format(
                "Attempted to write to ROM address {}\n", addr
            ));
        } else if(0xc000 <= addr && addr <= 0xdfff) {
            this->wram[addr - 0xc000] = data;
            return;
        } else if(0xff80 <= addr && addr <= 0xfffe) {
            this->hram[addr - 0xff80] = data;
            return;
        } else {
            throw std::logic_error(std::format(
                "Attempted to write to address {}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    };

    auto memory_read = [this](Address addr) -> const Byte& {
        if(0x0 <= addr && addr <= 0x3fff) {
            return this->rom.data[addr];
        } else if(0xc000 <= addr && addr <= 0xdfff) {
            return this->wram[addr - 0xc000];
        } else if(0xff80 <= addr && addr <= 0xfffe) {
            return this->hram[addr - 0xff80];
        } else {
            throw std::logic_error(std::format(
                "Attempted to read address {}. This address is either unimplemented or out of range.\n", addr
            ));
        }   
    };

    MemoryBus addressable_space{memory_write, memory_read};
    this->processor.addressable_space = addressable_space;
}

void GameBoy::run() {
    this->processor.fetch_decode_execute();
}   