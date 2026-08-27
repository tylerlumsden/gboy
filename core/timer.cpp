#include <stdexcept>
#include <format>

#include "timer.hpp"
#include "gameboy.hpp"

using GB::GameBoy;

bool control_enable(Byte control) {
    return control & 0b00000100;
}

// The gameboy internally checks if we should increment the counter by determining
// if the signal bit transitions from 1 -> 0 at any point.
// The signal bit is the bit that transitioning from 1 -> 0 would naively
// imply that the system counter has cycled enough times to trigger the counter,
// which is determined by the clock select.
// This behaviour has some annoying edge cases so it's easier to implement the gameboy
// internals directly than handle them one-by-one
template<typename Func>
void check_and_trigger_counter(Timer& timer, Func callback) {
    auto signal_bit_is_one = [](Timer& timer) {
        Byte clock_select = timer.control & 0b00000011;
        switch(clock_select) {
            case 0b00:
                // Return the 9th bit
                return (timer.system_counter >> 9) & 1;
            case 0b01:
                // Return the 3rd bit
                return (timer.system_counter >> 3) & 1;
            case 0b10:
                // Return the 5th bit
                return (timer.system_counter >> 5) & 1;
            case 0b11:
                // Return the 7th bit
                return (timer.system_counter >> 7) & 1;
            default:
                __builtin_unreachable();
        }
    };

    bool pre_signal_is_one = signal_bit_is_one(timer) && control_enable(timer.control);
    callback();
    bool post_signal_is_zero = !(signal_bit_is_one(timer) && control_enable(timer.control)); 

    if(pre_signal_is_one && post_signal_is_zero) {
        if(timer.counter == 0xff) {
            timer.overflow_flag = true;
        } 
        timer.counter += 1;
    }
}

void m_cycle_tick(GameBoy& gb, Byte count) {
    for(auto i = 0; i < count; ++i) {
        m_cycle_tick(gb);
    }
}

void m_cycle_tick(GameBoy& gb) {
    auto tick_func = [&gb]() {
        // 1 m_cycle = 4 t_cycles
        gb.timer.system_counter += 4;

        gb.timer.overflow_latch = false;
        if(gb.timer.overflow_flag) {
            gb.timer.counter = gb.timer.modulo;
            request_timer_interrupt(gb.interrupt);
            gb.timer.overflow_flag = false;
            gb.timer.overflow_latch = true;
        }
    };

    check_and_trigger_counter(gb.timer, tick_func);
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
            return timer.control | 0xf8;
        }
        throw std::invalid_argument(std::format(
            "TIMER: Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
        ));
    }
    void write(Timer& timer, Address addr, Byte data) {
        check_and_trigger_counter(timer, [&] {
            if(addr == 0xff04) {
                timer.system_counter = 0x0;
            } else if(addr == 0xff05) {
                if(!timer.overflow_latch) {
                    timer.overflow_flag = false;
                    timer.counter = data;
                }
            } else if(addr == 0xff06) {
                timer.modulo = data;
                if(timer.overflow_latch) {
                    timer.counter = data;
                }
            } else if(addr == 0xff07) {
                timer.control = data;
            } else {
                throw std::invalid_argument(std::format(
                    "TIMER: Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
                ));
            }
        });
    }
}
