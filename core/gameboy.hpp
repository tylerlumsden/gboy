#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "data_types.hpp"

#include<format>

namespace GB {

struct GameBoy {
    Cartridge cart;
    std::array<Byte, 0xffff> memory_map;

    SM83::CPU processor;

    void run();

    GameBoy(Cartridge cartridge);
    GameBoy() = delete;

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
            processor.A, processor.zero_flag, processor.subtraction_flag, processor.half_carry_flag, processor.carry_flag,
            processor.B, processor.C, processor.D, processor.E, processor.H, processor.L, processor.stack_pointer, processor.program_counter, processor.IME
        );
    }
};

Byte read(GameBoy& gb, Address addr);
void write(GameBoy& gb, Address addr, Byte data);

}

namespace SM83 {

void fetch_decode_execute(GB::GameBoy& gb);

}
