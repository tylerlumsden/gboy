#pragma once
#include <string>
#include <format>

#include "data_types.hpp"

struct Interrupt {
    Byte interrupt_flag = 0xe1;
    Byte interrupt_enable = 0x00;

    std::string print_state() {
        return std::format(R"(
            Interrupt State:
            interrupt_flag: {:#x}
            interrupt_enable: {:#x}
            )",
            interrupt_flag, interrupt_enable
        );
    }
};


inline void request_vblank_interrupt(Interrupt& interrupt) {
    interrupt.interrupt_flag |= 0b00000001;
}

inline void request_lcd_interrupt(Interrupt& interrupt) {
    interrupt.interrupt_flag |= 0b00000010;
}

inline void request_timer_interrupt(Interrupt& interrupt) {
    interrupt.interrupt_flag |= 0b00000100;
}

inline void request_serial_interrupt(Interrupt& interrupt) {
    interrupt.interrupt_flag |= 0b00001000;
}

inline void request_joypad_interrupt(Interrupt& interrupt) {
    interrupt.interrupt_flag |= 0b00010000;
}