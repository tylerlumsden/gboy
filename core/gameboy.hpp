#pragma once

#include "rom.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

struct GameBoy { 
    Rom rom;
    SM83::CPU processor;

    void run();
    Byte& memory_map(Address addr);

    GameBoy(Rom rom);
    GameBoy() = delete; 
};