#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

namespace GB {

struct GameBoy {
    Cartridge cart;
    std::array<Byte, 0xc000> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Cartridge cartridge);
    GameBoy() = delete;
};

Byte read (GameBoy& gb, Address addr);
void write(GameBoy& gb, Address addr, Byte data);

}

namespace SM83 {

void fetch_decode_execute(GB::GameBoy& gb);

}
