#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

struct GameBoy { 
    Cartridge cart;
    std::array<Byte, 0xc000> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Cartridge cartridge);
    GameBoy() = delete; 
};