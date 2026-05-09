#pragma once

#include <stdexcept>
#include <array>
#include <format>

#include "memory.hpp"
#include "data_types.hpp"

namespace SM83 {

struct CPU {

    MemoryBus addressable_space;

    // Instruction Definitions
    using InstructionFunc = void (CPU::*)();

    void fetch_decode_execute();

    template<Byte Opcode>
    void no_impl() {
        throw std::logic_error(std::format("Opcode with byte value {:#x} is not implemented.\n", Opcode));
    }

    static constexpr std::array<InstructionFunc, 256> instruction_handler = [](){
        // Default initialize the handler array with all instructions not implemented
        std::array<InstructionFunc, 256> handler = 
        []<std::size_t... I>(std::index_sequence<I...>) {
            return std::array<InstructionFunc, 256>{ &CPU::template no_impl<I>... };
        }(std::make_index_sequence<256>{});

        // Initialize each instruction manually here

        return handler;
    }();
};
}