#pragma once

#include "rom.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

struct GameBoy { 
    Rom rom;
    std::array<Byte, 0xc000> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Rom rom);
    GameBoy() = delete; 
};