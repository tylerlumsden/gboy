#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "timer.hpp"
#include "interrupt.hpp"
#include "data_types.hpp"

namespace GB {

struct GameBoy {
    Cartridge cart;
    Timer timer;
    Interrupt interrupt;
    std::array<Byte, 0xffff> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Cartridge cartridge);
    GameBoy() = delete;
};

Byte read(GameBoy& gb, Address addr);
void write(GameBoy& gb, Address addr, Byte data);

}

namespace SM83 {

void fetch_decode_execute(GB::GameBoy& gb);

}
