#include "gameboy.hpp"
#include "ppu.hpp"

using GB::GameBoy;

Double_Byte resolve_line_y(Quad_Byte dots_elapsed) {
    return dots_elapsed / 456;
}

Double_Byte resolve_line_dots(Quad_Byte dots_elapsed) {
    return dots_elapsed % 456;
}

void ppu_dot_state_machine(GameBoy& gb) {

    auto line_y = resolve_line_y(gb.ppu.state.dots_elapsed);
    auto line_dots = resolve_line_dots(gb.ppu.state.dots_elapsed);

    // State machine as per https://gbdev.io/pandocs/Rendering.html
    if(gb.ppu.state.mode == PPU_Mode::OAM) {


        // Edge transition
        if(line_dots == 79) {
            gb.ppu.state.mode = PPU_Mode::DRAW;
        }
    }
    else if(gb.ppu.state.mode == PPU_Mode::DRAW) {
        // Temporary edge transition
        if(line_dots == 368) {
            gb.ppu.state.mode = PPU_Mode::HBLANK;
        }
    }
    else if(gb.ppu.state.mode == PPU_Mode::HBLANK) {
        // Edge transition
        if(line_dots == 455 && line_y == 143) {
            gb.ppu.state.mode = PPU_Mode::VBLANK;

            gb.ppu.frame_buffer_callback(gb.ppu.buffer);
        }
        else if(line_dots == 455) {
            gb.ppu.state.mode = PPU_Mode::OAM;
        }
    }
    else if(gb.ppu.state.mode == PPU_Mode::VBLANK) {
        // Edge transition
        if(line_y == 153 && line_dots == 455) {
            gb.ppu.state.mode = PPU_Mode::OAM;
        }
    }

    gb.ppu.state.dots_elapsed += 1;
    if(gb.ppu.state.dots_elapsed == 70224) {
        gb.ppu.state.dots_elapsed = 0;
    }
}