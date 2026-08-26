#include <stdexcept>
#include <format>

#include "timer.hpp"
#include "gameboy.hpp"

using GB::GameBoy;

Double_Byte control_increment(Byte control) {
    Byte clock_select = control & 0b00000011;
    switch(clock_select) {
    case 0b00:
        return 256;
    case 0b01:
        return 4;
    case 0b10:
        return 16;
    case 0b11:
        return 64;
    default:
        __builtin_unreachable();
    }
}

bool control_enable(Byte control) {
    return control & 0b00000100;
}

void m_cycle_tick(GameBoy& gb, Byte count) {
    for(auto i = 0; i < count; ++i) {
        m_cycle_tick(gb);
    }
}

void m_cycle_tick(GameBoy& gb) {
    // 1 m_cycle = 4 t_cycles
    gb.timer.system_counter += 4;

    bool timer_enable = control_enable(gb.timer.control);

    if(timer_enable) {
        if(gb.timer.overflow_flag) {

            gb.timer.counter = gb.timer.modulo;
            request_timer_interrupt(gb.interrupt);
            gb.timer.overflow_flag = false;

        } else {

            Double_Byte timer_increment = control_increment(gb.timer.control);
            if(gb.timer.system_counter % timer_increment == 0) {
                if(gb.timer.counter == 0xff) {
                    gb.timer.overflow_flag = true;
                } else {
                    gb.timer.counter += 1;
                }
            }
            
        }
    }
}

namespace TIMER {
    Byte read(Timer& timer, Address addr) {
        if(addr == 0xff04) {
            return hi(timer.system_counter);
        } else if(addr == 0xff05) {
            return timer.counter;
        } else if(addr == 0xff06) {
            return timer.modulo;
        } else if(addr == 0xff07) {
            return timer.control;
        }
        throw std::invalid_argument(std::format(
            "TIMER: Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
    }
    void write(Timer& timer, Address addr, Byte data) {
        if(addr == 0xff04) {
            timer.system_counter = timer.system_counter & 0x00ff;
        } else if(addr == 0xff05) {
            timer.counter = data;
        } else if(addr == 0xff06) {
            timer.modulo = data;
        } else if(addr == 0xff07) {
            timer.control = data;
        } else {
            throw std::invalid_argument(std::format(
                "TIMER: Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    }
}
