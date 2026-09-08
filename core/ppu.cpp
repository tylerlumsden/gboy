#include "gameboy.hpp"
#include "ppu.hpp"
#include "memory.hpp"

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
        if(line_dots == 0) {
            // Loop through all entries in the OAM and record all collisions on the current line
            Byte oam_list_size = 0;
            for(Address offset = 0; offset < 160 && oam_list_size < 10; offset += 4) {
                // As per pandocs, the position is the actual position + 16
                Byte oam_position_y = gb.ppu.oam[offset] - 16;

                // TODO: Get the tile height from the LCD
                Byte tile_height = 8;
                if(oam_position_y <= line_y && line_y <= oam_position_y + tile_height) {
                    Byte oam_position_x = gb.ppu.oam[offset + 1];
                    Byte oam_tile_index = gb.ppu.oam[offset + 2];
                    Byte oam_attribute = gb.ppu.oam[offset + 3];

                    gb.ppu.state.oam_buffer[oam_list_size] = {oam_position_y, oam_position_x, oam_tile_index, oam_attribute};
                    ++oam_list_size;
                }
            }
        }
        // Edge transition
        if(line_dots == 79) {
            gb.ppu.state.mode = PPU_Mode::DRAW;
        }
    }
    else if(gb.ppu.state.mode == PPU_Mode::DRAW) {
        if(line_dots == 80) {
            for(auto entry : gb.ppu.state.oam_buffer) {
                // Need to bounds check the pixel_x here
                Byte pixel_x = entry[1] - 8;
                Quad_Byte pixel_index = (line_y * GB_Width) + pixel_x;
                gb.ppu.buffer[pixel_index] = 0xFFFFFFFF; // black pixel
            }
        }

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

namespace PPU {
    Byte read(PPU_Data& ppu, Address addr) {
        if(0xfe00 <= addr && addr <= 0xfe9f) {
            return ppu.oam[addr - 0xfe00];
        } 
        else {
            throw std::invalid_argument(std::format(
                "PPU: Attempted to read address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
        
    }
    void write(PPU_Data& ppu, Address addr, Byte data) {
        if(0xfe00 <= addr && addr <= 0xfe9f) {
            ppu.oam[addr - 0xfe00] = data;
        }
        else {
            throw std::invalid_argument(std::format(
                "PPU: Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    }
}