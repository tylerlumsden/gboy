#pragma once

#include <functional>
#include <array>

#include "data_types.hpp"
#include "display.hpp"

namespace GB { struct GameBoy; }

enum PPU_Mode {
    HBLANK,
    VBLANK,
    OAM,
    DRAW
};

using OAM_Entry = std::array<Byte, 4>;
struct PPU_State {
    PPU_Mode mode = PPU_Mode::OAM;
    Quad_Byte dots_elapsed = 0;
    std::array<OAM_Entry, 10> oam_buffer;
};

struct PPU_Data {
    FrameBuffer buffer;
    FrameBufferCallback frame_buffer_callback;
    PPU_State state;
    std::array<Byte, 0xa0> oam;
}; 

namespace PPU {
    Byte read(PPU_Data& ppu, Address addr);
    void write(PPU_Data& ppu, Address addr, Byte data);
};

void ppu_dot_state_machine(GB::GameBoy& gb);