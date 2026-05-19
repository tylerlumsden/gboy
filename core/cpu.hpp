#pragma once

#include "memory.hpp"
#include "data_types.hpp"
#include <format>
#include <string>

template <auto Val, auto... Candidates>
constexpr bool is_one_of = ((Val == Candidates) || ...);

namespace SM83 {

struct CPU {
    // Registers and flags
    Byte A = 0x01;
    bool zero_flag = 0x1;
    bool subtraction_flag = 0x0;
    // For the DMG gameboy, the boot state of the carry flags depend
    // on the checksum of the ROM. If this is an issue, we need to add logic for these
    bool half_carry_flag = 0x0;
    bool carry_flag = 0x0;
    Byte B = 0xff;
    Byte C = 0x13;
    Byte D = 0x00;
    Byte E = 0xc1;
    Byte H = 0x84;
    Byte L = 0x03;
    Address stack_pointer = 0xfffe;
    Address program_counter = 0x100;

    bool IME = 0x0;

    MemoryBus addressable_space;

    void fetch_decode_execute();

    std::string print_state() {
        return std::format(R"(
            CPU State:
            A: {:#x}
            zero_flag: {:#x}
            subtraction_flag: {:#x}
            half_carry_flag: {:#x}
            carry_flag: {:#x}
            B: {:#x}
            C: {:#x}
            D: {:#x}
            E: {:#x}
            H: {:#x}
            L: {:#x}
            stack_pointer: {:#x}
            program_counter: {:#x}
            IME: {:#x}
            )",
            A, zero_flag, subtraction_flag, half_carry_flag, carry_flag,
            B, C, D, E, H, L, stack_pointer, program_counter, IME
        );
    }
};

}
