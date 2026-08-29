#pragma once

#include "data_types.hpp"

#include <format>

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
    bool half_carry_flag = 0x1;
    bool carry_flag = 0x1;
    Byte B = 0x00;
    Byte C = 0x13;
    Byte D = 0x00;
    Byte E = 0xd8;
    Byte H = 0x01;
    Byte L = 0x4d;
    Address stack_pointer = 0xfffe;
    Address program_counter = 0x100;

    bool IME = false;
    bool halt_mode = false;
    bool stop_mode = false;
};

}

template <>
struct std::formatter<SM83::CPU> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const SM83::CPU& cpu, std::format_context& ctx) const {
        return std::format_to(ctx.out(), R"(
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
            cpu.A, cpu.zero_flag, cpu.subtraction_flag, cpu.half_carry_flag, cpu.carry_flag,
            cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.stack_pointer, cpu.program_counter, cpu.IME
        );
    }
};
