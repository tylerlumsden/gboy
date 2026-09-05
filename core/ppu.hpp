#pragma once

#include <functional>
#include <array>

#include "data_types.hpp"

namespace GB { struct GameBoy; }

enum PPU_Mode {
    HBLANK,
    VBLANK,
    OAM,
    DRAW
};

struct PPU_State {
    PPU_Mode mode = PPU_Mode::OAM;
    Quad_Byte dots_elapsed = 0;
};

using FrameBuffer = std::array<Quad_Byte, 23040>;
using FrameBufferCallback = std::function<void(FrameBuffer&)>;
struct PPU {
    // The gameboy has 23040 pixels total
    FrameBuffer buffer;
    FrameBufferCallback frame_buffer_callback;
    PPU_State state;
}; 

void ppu_dot_state_machine(GB::GameBoy& gb);