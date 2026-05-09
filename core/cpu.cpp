#include "cpu.hpp"  
#include "data_types.hpp"
#include "log.hpp"

void SM83::CPU::fetch_decode_execute() {
    constexpr Address entry = 0x100;

    Log::log(R"(Entry point:      
        0x100: {:#x}
        0x101: {:#x}
        0x102: {:#x}
        0x103: {:#x})", addressable_space[entry], addressable_space[entry + 1], addressable_space[entry + 2], addressable_space[entry + 3]);
}