#pragma once

#include "data_types.hpp"

struct Interrupt {
    Byte interrupt_flag = 0xe1;
    Byte interrupt_enable = 0x00;
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