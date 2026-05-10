#pragma once

#include "rom.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

struct GameBoy { 
    std::array<Byte, 0x2000> wram;
    std::array<Byte, 0x7F> hram;

    Rom rom;
    SM83::CPU processor;

    void run();

    GameBoy(Rom rom);
    GameBoy() = delete; 
};