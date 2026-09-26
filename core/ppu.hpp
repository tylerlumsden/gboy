#pragma once

#include <functional>
#include <array>

#include "data_types.hpp"
#include "display.hpp"

namespace GB { struct GameBoy; }

struct LCD {
    Byte control = 0x91;
    Byte status = 0x85;
    Byte line_y = 0x00;
    Byte line_y_compare = 0x00;
    Byte scroll_y = 0x00;
    Byte scroll_x = 0x00;
    Byte background_palette = 0xfc;

    Byte object_palette1 = 0xfc;
    Byte object_palette2 = 0xfc;

    Byte window_line_y = 0;
    Byte window_y = 0;
    Byte window_x = 7;
};

struct DMA_State {
    Byte ticks_left = 0;
    Address address_offset = 0;
};

using OAM_Entry = std::array<Byte, 4>;
struct PPU_State {
    Quad_Byte dots_elapsed = 0;
    std::array<OAM_Entry, 10> oam_buffer;
    DMA_State dma_state; 
};

struct PPU_Data {
    FrameBuffer buffer;
    FrameBufferCallback frame_buffer_callback;
    PPU_State state;
    LCD lcd;
    std::array<Byte, 0xa0> oam;
}; 

namespace PPU {
    Byte read(PPU_Data& ppu, Address addr);
    void write(PPU_Data& ppu, Address addr, Byte data);
};

void ppu_dot_state_machine(GB::GameBoy& gb);

void debug_render_tileset(GB::GameBoy& gb, TilemapBuffer& buffer);