#pragma once

#include "data_types.hpp"

namespace GB { struct GameBoy; }

struct Timer {
    Quad_Byte cycles_per_second = 256;
    Double_Byte system_counter = 0xab00;
    Byte counter = 0;
    Byte modulo = 0;
    Byte control = 0xf8;


};

void m_cycle_tick(GB::GameBoy& gb);

namespace TIMER {
    Byte read(Timer& timer, Address addr);
    void write(Timer& timer, Address addr, Byte data);
}