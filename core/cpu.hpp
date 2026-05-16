#pragma once

#include <stdexcept>
#include <array>
#include <format>

#include "memory.hpp"
#include "data_types.hpp"

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
    Address stack_pointer = 0xFFFE;
    Address program_counter = 0x100;

    bool IME = 0x0;

    MemoryBus addressable_space;
    Byte fetch();
    void fetch_decode_execute();

    void push_program_counter();
    void pop_program_counter();

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
    
    template<Byte Opcode>
    void inc();

    template<Byte Opcode>
    void ret();

    template<Byte Opcode>
    void ld_register();

    template<Byte Opcode> 
        requires is_one_of<Opcode, 0x01, 0x11, 0x21, 0x31>
    void ld_n16();

    template <Byte Opcode>
    void ld();

    template<Byte Opcode>
    void ldh();

    template<Byte Opcode>
        requires is_one_of<Opcode, 0xc4, 0xd4, 0xcc, 0xdc, 0xcd>
    void call();

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
        handler[0x31] = &CPU::ld<0x31>;
        handler[0xff] = &CPU::rst<0xff>;
        handler[0x3c] = &CPU::inc<0x3c>;
        handler[0xc9] = &CPU::ret<0xc9>;
        handler[0xea] = &CPU::ld<0xea>;
        handler[0x3e] = &CPU::ld<0x3e>;
        handler[0xe0] = &CPU::ldh<0xe0>;

        // call
        handler[0xc4] = &CPU::call<0xc4>;
        handler[0xd4] = &CPU::call<0xd4>;
        handler[0xcc] = &CPU::call<0xcc>;
        handler[0xdc] = &CPU::call<0xdc>;
        handler[0xcd] = &CPU::call<0xcd>;

        // ld_n16
        handler[0x01] = &CPU::ld_n16<0x01>;
        handler[0x11] = &CPU::ld_n16<0x11>;
        handler[0x21] = &CPU::ld_n16<0x21>;
        handler[0x31] = &CPU::ld_n16<0x31>;

        // ld_register
        handler[0x40] = &CPU::ld_register<0x40>;
        handler[0x41] = &CPU::ld_register<0x41>;
        handler[0x42] = &CPU::ld_register<0x42>;
        handler[0x43] = &CPU::ld_register<0x43>;
        handler[0x44] = &CPU::ld_register<0x44>;
        handler[0x45] = &CPU::ld_register<0x45>;
        handler[0x46] = &CPU::ld_register<0x46>;
        handler[0x47] = &CPU::ld_register<0x47>;
        handler[0x48] = &CPU::ld_register<0x48>;
        handler[0x49] = &CPU::ld_register<0x49>;
        handler[0x4a] = &CPU::ld_register<0x4a>;
        handler[0x4b] = &CPU::ld_register<0x4b>;
        handler[0x4c] = &CPU::ld_register<0x4c>;
        handler[0x4d] = &CPU::ld_register<0x4d>;
        handler[0x4e] = &CPU::ld_register<0x4e>;
        handler[0x4f] = &CPU::ld_register<0x4f>;
        handler[0x50] = &CPU::ld_register<0x50>;
        handler[0x51] = &CPU::ld_register<0x51>;
        handler[0x52] = &CPU::ld_register<0x52>;
        handler[0x53] = &CPU::ld_register<0x53>;
        handler[0x54] = &CPU::ld_register<0x54>;
        handler[0x55] = &CPU::ld_register<0x55>;
        handler[0x56] = &CPU::ld_register<0x56>;
        handler[0x57] = &CPU::ld_register<0x57>;
        handler[0x58] = &CPU::ld_register<0x58>;
        handler[0x59] = &CPU::ld_register<0x59>;
        handler[0x5a] = &CPU::ld_register<0x5a>;
        handler[0x5b] = &CPU::ld_register<0x5b>;
        handler[0x5c] = &CPU::ld_register<0x5c>;
        handler[0x5d] = &CPU::ld_register<0x5d>;
        handler[0x5e] = &CPU::ld_register<0x5e>;
        handler[0x5f] = &CPU::ld_register<0x5f>;
        handler[0x60] = &CPU::ld_register<0x60>;
        handler[0x61] = &CPU::ld_register<0x61>;
        handler[0x62] = &CPU::ld_register<0x62>;
        handler[0x63] = &CPU::ld_register<0x63>;
        handler[0x64] = &CPU::ld_register<0x64>;
        handler[0x65] = &CPU::ld_register<0x65>;
        handler[0x66] = &CPU::ld_register<0x66>;
        handler[0x67] = &CPU::ld_register<0x67>;
        handler[0x68] = &CPU::ld_register<0x68>;
        handler[0x69] = &CPU::ld_register<0x69>;
        handler[0x6a] = &CPU::ld_register<0x6a>;
        handler[0x6b] = &CPU::ld_register<0x6b>;
        handler[0x6c] = &CPU::ld_register<0x6c>;
        handler[0x6d] = &CPU::ld_register<0x6d>;
        handler[0x6e] = &CPU::ld_register<0x6e>;
        handler[0x6f] = &CPU::ld_register<0x6f>;
        handler[0x70] = &CPU::ld_register<0x70>;
        handler[0x71] = &CPU::ld_register<0x71>;
        handler[0x72] = &CPU::ld_register<0x72>;
        handler[0x73] = &CPU::ld_register<0x73>;
        handler[0x74] = &CPU::ld_register<0x74>;
        handler[0x75] = &CPU::ld_register<0x75>;
        // 0x76 is skipped -- it is a halt instruction
        handler[0x77] = &CPU::ld_register<0x77>;
        handler[0x78] = &CPU::ld_register<0x78>;
        handler[0x79] = &CPU::ld_register<0x79>;
        handler[0x7a] = &CPU::ld_register<0x7a>;
        handler[0x7b] = &CPU::ld_register<0x7b>;
        handler[0x7c] = &CPU::ld_register<0x7c>;
        handler[0x7d] = &CPU::ld_register<0x7d>;
        handler[0x7e] = &CPU::ld_register<0x7e>;
        handler[0x7f] = &CPU::ld_register<0x7f>;

        // Initialize each instruction manually here

        return handler;
    }();
};
}