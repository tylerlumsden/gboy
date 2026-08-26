#pragma once

#include <format>
#include <string>

#include "data_types.hpp"

namespace GB { struct GameBoy; }

struct Timer {
    Quad_Byte cycles_per_second = 256;
    Double_Byte system_counter = 0xab00;
    Byte counter = 0;
    Byte modulo = 0;
    Byte control = 0xf8;
    bool overflow_flag = false;

    std::string print_state() {
        return std::format(R"(
            Timer State:
            cycles_per_second: {}
            system_counter: {:#x}
            counter: {:#x}
            modulo: {:#x}
            control: {:#x}
            overflow: {}
            )",
            cycles_per_second, system_counter, counter, modulo, control, overflow_flag
        );
    }
};

void m_cycle_tick(GB::GameBoy& gb);
void m_cycle_tick(GB::GameBoy& gb, Byte count);

namespace TIMER {
    Byte read(Timer& timer, Address addr);
    void write(Timer& timer, Address addr, Byte data);
}