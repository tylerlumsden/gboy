#pragma once

#include "rom.hpp"

struct GameBoy {    
    Rom rom;

    void run();

    GameBoy(Rom rom) : rom(rom) {}
    GameBoy() = delete; 
};