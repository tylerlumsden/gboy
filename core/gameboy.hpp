#pragma once

#include "cartridge.hpp"
#include "ppu.hpp"
#include "cpu.hpp"
#include "timer.hpp"
#include "interrupt.hpp"
#include "data_types.hpp"

namespace GB {

struct GameBoy {
    Cartridge cart;
    PPU_Data ppu;
    Timer timer;
    Interrupt interrupt;
    std::array<Byte, 0x2000> vram;
    std::array<Byte, 0x1000> main_wram;
    std::array<Byte, 0x1000> banked_wram;

    std::array<Byte, 0xa0> oam;

    std::array<Byte, 0x7f> hram;

    std::array<Byte, 0xffff> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Cartridge cartridge, FrameBufferCallback callback);
    GameBoy() = delete;
};

Byte read(GameBoy& gb, Address addr);
void write(GameBoy& gb, Address addr, Byte data);

}

namespace SM83 {

void fetch_decode_execute(GB::GameBoy& gb);

}
