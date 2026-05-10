#pragma once

#include <stdexcept>
#include <array>
#include <format>

#include "memory.hpp"
#include "data_types.hpp"

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
    Double_Byte BC = 0xFF13;
    Double_Byte DE = 0x00C1;
    Double_Byte HL = 0x8403;
    Double_Byte stack_pointer = 0xFFFE;
    Address program_counter = 0x100;

    bool IME = 0x0;

    MemoryBus addressable_space;
    Byte fetch();
    void fetch_decode_execute();

    // Instruction Definitions
    using InstructionFunc = void (CPU::*)();
    template<Byte Opcode>
    void no_impl() {
        throw std::logic_error(std::format("Opcode with byte value {:#x} is not implemented.\n", Opcode));
    }

    void nop();
    void jp();
    void cp();

    template <Byte Opcode>
    void jr();

    template <Byte Opcode>
    void x_or();

    void di();

    template<Byte Opcode>
    void swap();

    template<Byte Opcode>
    void rst();

    static constexpr std::array<InstructionFunc, 256> instruction_handler = [](){
        // Default initialize the handler array with all instructions not implemented
        std::array<InstructionFunc, 256> handler = 
        []<std::size_t... I>(std::index_sequence<I...>) {
            return std::array<InstructionFunc, 256>{ &CPU::template no_impl<I>... };
        }(std::make_index_sequence<256>{});

        handler[0x0] = &CPU::nop;
        handler[0xc3] = &CPU::jp;
        handler[0xfe] = &CPU::cp;
        handler[0x28] = &CPU::jr<0x28>;
        handler[0xaf] = &CPU::x_or<0xaf>;
        handler[0x18] = &CPU::jr<0x18>;
        handler[0xf3] = &CPU::di;
        handler[0x31] = &CPU::swap<0x31>;
        handler[0xff] = &CPU::rst<0xff>;

        // Initialize each instruction manually here

        return handler;
    }();
};
}