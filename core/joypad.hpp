#pragma once

#include "data_types.hpp"

struct Joypad {
    Byte joypad_register = 0xcf;
};

namespace JOY {

inline void write(Joypad& joypad, Byte data) {
    Byte low_nibble = joypad.joypad_register & 0b00001111;
    Byte high_nibble = data & 0b11110000;
    joypad.joypad_register = high_nibble | low_nibble;
}

inline void clear_all(Joypad& joypad) {
    joypad.joypad_register |= 0b00001111;
}

inline void up(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 4)) {
        clear_bit(joypad.joypad_register, 2);
    }
}

inline void down(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 4)) {
        clear_bit(joypad.joypad_register, 3);
    }
}

inline void left(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 4)) {
        clear_bit(joypad.joypad_register, 1);
    }
}

inline void right(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 4)) {
        clear_bit(joypad.joypad_register, 0);
    }
}

inline void select(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 5)) {
        clear_bit(joypad.joypad_register, 2);
    }
}

inline void start(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 5)) {
        clear_bit(joypad.joypad_register, 3);
    }
}

inline void a(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 5)) {
        clear_bit(joypad.joypad_register, 0);
    }
}

inline void b(Joypad& joypad) {
    if(!get_bit(joypad.joypad_register, 5)) {
        clear_bit(joypad.joypad_register, 1);
    }
}

}