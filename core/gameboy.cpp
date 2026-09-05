#include "gameboy.hpp"
#include "data_types.hpp"
#include "log.hpp"

#include <format>
#include <stdexcept>

namespace GB {

GameBoy::GameBoy(Cartridge cartridge, FrameBufferCallback callback) : cart(cartridge) {
    this->ppu.frame_buffer_callback = callback;
}

Byte read(GameBoy& gb, Address addr) {
    Log::log<Log::Level::Debug>("Reading from memory address {:#x}", addr);
    if(0x0 <= addr && addr <= 0x7fff) {
        return CART::read(gb.cart, addr);
    }
    else if(0x8000 <= addr && addr <= 0x9fff) {
        return gb.vram[addr - 0x8000];
    }
    else if(0xa000 <= addr && addr <= 0xbfff) {
        return CART::read(gb.cart, addr);
    }
    else if(0xc000 <= addr && addr <= 0xcfff) {
        return gb.main_wram[addr - 0xc000];
    }
    else if(0xd000 <= addr && addr <= 0xdfff) {
        return gb.banked_wram[addr - 0xd000];
    }
    else if(0xff04 <= addr && addr <= 0xff07) {
        return TIMER::read(gb.timer, addr);
    }
    else if(addr == 0xff44) {
        // Placeholder for the LY until it gets implemented
        return 0x90;
    }
    else if(addr == 0xff0f) {
        return gb.interrupt.interrupt_flag;
    }
    else if(0xff4c <= addr && addr <= 0xff4f) {
        // returns garbage, unimplemented range for the DMG
        return 0xff;
    }
    else if(0xff80 <= addr && addr <= 0xfffe) {
        return gb.hram[addr - 0xff80];
    }
    else if(addr == 0xffff) {
        return gb.interrupt.interrupt_enable;
    }
    else {
        Log::log<Log::Level::Error>("Reading from unhandled memory address {:#x}", addr);
        return gb.memory_map[addr];
        /*
        throw std::logic_error(std::format(
            "Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
        */
    }
}

void write(GameBoy& gb, Address addr, Byte data) {
    Log::log<Log::Level::Debug>("Writing to memory address {:#x} with data {}", addr, data);
    if(0x0 <= addr && addr <= 0x7fff) {
        CART::write(gb.cart, addr, data);
    }
    else if(0x8000 <= addr && addr <= 0x9fff) {
        gb.vram[addr - 0x8000] = data;
    }
    else if(0xa000 <= addr && addr <= 0xbfff) {
        CART::write(gb.cart, addr, data);
    }
    else if(0xc000 <= addr && addr <= 0xcfff) {
        gb.main_wram[addr - 0xc000] = data;
    }
    else if(0xd000 <= addr && addr <= 0xdfff) {
        gb.banked_wram[addr - 0xd000] = data;
    }
    else if(addr == 0xff01) {
        std::cout << data;
    }
    else if(0xff04 <= addr && addr <= 0xff07) {
        TIMER::write(gb.timer, addr, data);
    }
    else if(addr == 0xff0f) {
        gb.interrupt.interrupt_flag = data;
    }
    else if(0xff4c <= addr && addr <= 0xff4f) {
        // no-op, unimplemented range for the DMG
    }
    else if(0xff80 <= addr && addr <= 0xfffe) {
        gb.hram[addr - 0xff80] = data;
    }
    else if(addr == 0xffff) {
        gb.interrupt.interrupt_enable = data;
    }
    else {
        Log::log<Log::Level::Error>("Writing to unhandled memory address {:#x} with data {}", addr, data);
        gb.memory_map[addr] = data;
        /*
        throw std::logic_error(std::format(
            "Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
        */
    }
}

void GameBoy::run() {
    SM83::fetch_decode_execute(*this);
}

}
