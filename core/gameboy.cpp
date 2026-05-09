#include "gameboy.hpp"
#include "data_types.hpp"
#include "log.hpp"

void GameBoy::run() {
    constexpr Address entry = 0x100;

    Log::log(R"(Entry point:      
        0x100: {:#x}
        0x101: {:#x}
        0x102: {:#x}
        0x103: {:#x})", rom.data[entry], rom.data[entry + 1], rom.data[entry + 2], rom.data[entry + 3]);
}   