#pragma once

#include "data_types.hpp"
#include "interrupt.hpp"
#include "log.hpp"

struct Joypad {
    Byte dpad_pressed = 0xff;
    Byte buttons_pressed = 0xff;
    Byte joypad_register = 0xcf;
};

namespace JOY {

inline void write(Joypad& joypad, Byte data) {
    Byte low_nibble = joypad.joypad_register & 0b00001111;
    Byte high_nibble = data & 0b11110000;
    joypad.joypad_register = high_nibble | low_nibble;
}

inline Byte read(Joypad& joypad) {
    Log::log<Log::Level::Debug>("Joypad state: {:#b}", joypad.joypad_register);

    Byte low_nibble = 0b00001111;

    if(!get_bit(joypad.joypad_register, 5)) {
        low_nibble &= joypad.buttons_pressed;
    }

    if(!get_bit(joypad.joypad_register, 4)) {
        low_nibble &= joypad.dpad_pressed;
    }

    joypad.joypad_register = (joypad.joypad_register & 0b11110000) | low_nibble;

    return joypad.joypad_register;
}

inline void clear_all(Joypad& joypad) {
    joypad.dpad_pressed |= 0b00001111;
    joypad.buttons_pressed |= 0b00001111;
}

inline void up(Joypad& joypad) {
    joypad.dpad_pressed = clear_bit(joypad.dpad_pressed, 2);
}

inline void down(Joypad& joypad) {
    joypad.dpad_pressed = clear_bit(joypad.dpad_pressed, 3);
}

inline void left(Joypad& joypad) {
    joypad.dpad_pressed = clear_bit(joypad.dpad_pressed, 1);
}

inline void right(Joypad& joypad) {
    joypad.dpad_pressed = clear_bit(joypad.dpad_pressed, 0);
}

inline void select(Joypad& joypad) {
    joypad.buttons_pressed = clear_bit(joypad.buttons_pressed, 2);
}

inline void start(Joypad& joypad) {
    joypad.buttons_pressed = clear_bit(joypad.buttons_pressed, 3);
}

inline void a(Joypad& joypad) {
    joypad.buttons_pressed = clear_bit(joypad.buttons_pressed, 0);
}

inline void b(Joypad& joypad) {
    joypad.buttons_pressed = clear_bit(joypad.buttons_pressed, 1);
}

inline void up(Joypad& joypad, Interrupt& interrupt) {
    up(joypad);
    request_joypad_interrupt(interrupt);
}

inline void down(Joypad& joypad, Interrupt& interrupt) {
    down(joypad);
    request_joypad_interrupt(interrupt);
}

inline void left(Joypad& joypad, Interrupt& interrupt) {
    left(joypad);
    request_joypad_interrupt(interrupt);
}

inline void right(Joypad& joypad, Interrupt& interrupt) {
    right(joypad);
    request_joypad_interrupt(interrupt);
}

inline void select(Joypad& joypad, Interrupt& interrupt) {
    select(joypad);
    request_joypad_interrupt(interrupt);
}

inline void start(Joypad& joypad, Interrupt& interrupt) {
    start(joypad);
    request_joypad_interrupt(interrupt);
}

inline void a(Joypad& joypad, Interrupt& interrupt) {
    a(joypad);
    request_joypad_interrupt(interrupt);
}

inline void b(Joypad& joypad, Interrupt& interrupt) {
    b(joypad);
    request_joypad_interrupt(interrupt);
}

}