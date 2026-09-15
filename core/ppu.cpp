#include "gameboy.hpp"
#include "ppu.hpp"
#include "memory.hpp"

#include "log.hpp"

using GB::GameBoy;

Double_Byte resolve_line_y(Quad_Byte dots_elapsed) {
    return dots_elapsed / 456;
}

Double_Byte resolve_line_dots(Quad_Byte dots_elapsed) {
    return dots_elapsed % 456;
}

std::pair<std::array<OAM_Entry, 10>, Byte> oam_scan(std::array<Byte, 0xa0> oam, Double_Byte line_y) {
    // Loop through all entries in the OAM and record all collisions on the current line

    std::array<OAM_Entry, 10> oam_buffer;
    Byte oam_list_size = 0;

    for(Address offset = 0; offset < 160 && oam_list_size < 10; offset += 4) {
        // As per pandocs, the position is the actual position + 16
        Byte oam_position_y = oam[offset] - 16;

        // TODO: Get the tile height from the LCD
        Byte tile_height = 8;
        if(oam_position_y <= line_y && line_y <= oam_position_y + tile_height) {
            Byte oam_position_x = oam[offset + 1];
            Byte oam_tile_index = oam[offset + 2];
            Byte oam_attribute = oam[offset + 3];

            oam_buffer[oam_list_size] = {oam_position_y, oam_position_x, oam_tile_index, oam_attribute};
            ++oam_list_size;
        }
    }

    return {oam_buffer, oam_list_size};
}

Quad_Byte background_color_map(Byte background_palette, Byte color_id) {
    Byte shade;
    switch(color_id) {
    case 0:
        shade = (background_palette & 0b11);
        break;
    case 1:
        shade = ((background_palette >> 2) & 0b11);
        break;
    case 2:
        shade = ((background_palette >> 4) & 0b11);
        break;
    case 3:
        shade = ((background_palette >> 6) & 0b11);
        break;
    default:
        throw std::invalid_argument(
            std::format("background_color_map: invalid color_id provided: {}", color_id)
        );
    }

    switch(shade) {
    // White
    case 0:
        return 0xFFFFFFFF;
    // Light grey
    case 1:
        return 0xFFD3D3D3;
    // Dark grey
    case 2:
        return 0xFF5A5A5A;
    // Black
    case 3:
        return 0xFF000000;
    default:
        __builtin_unreachable();
    }
}

void draw_background_line(GameBoy& gb) {
    auto lcdc = bit_array(gb.ppu.lcd.control);
    Address map_offset = 0x9800;
    if(lcdc[3]) {
        map_offset = 0x9c00;
    }

    Byte line_y = (gb.ppu.lcd.line_y + gb.ppu.lcd.scroll_y) % 256;
    Byte tile_row = (line_y % 8) * 2;
    Byte map_row = (line_y / 8) * 32;

    Byte line_x = gb.ppu.lcd.scroll_x;
    Byte render_x = -(line_x % 8);

    while(render_x < 160) {
        Address map_address = map_offset + map_row + ((line_x / 8) % 256);
        Byte tile_index = memory_bus(gb, map_address);

        Address tile_address;
        if(lcdc[4]) {
            tile_address = 0x8000 + (tile_index * 16) + tile_row;
        } else {
            tile_address = 0x9000 + static_cast<Signed_Byte>(tile_index * 16) + tile_row;
        }

        Byte low_data = memory_bus(gb, tile_address);
        Byte high_data = memory_bus(gb, tile_address + 1);

        for(Byte pixel = 0; pixel < 8; ++pixel) {
            if(0 <= render_x && render_x < 160) {
                Byte color_id = ((high_data >> pixel) & 0x1) << 1 | ((low_data >> pixel) & 0x1);

                Quad_Byte color = background_color_map(gb.ppu.lcd.background_palette, color_id);
                gb.ppu.buffer[gb.ppu.lcd.line_y * GB_Width + render_x] = color;
            }
            render_x += 1;
        }
        line_x += 8;
    }
}

void ppu_draw_line(GameBoy& gb) {

    draw_background_line(gb);

    /*
    auto [oam_buffer, oam_list_size] = oam_scan(gb.ppu.oam, gb.ppu.lcd.line_y);
    for(Byte i = 0; i < oam_list_size; ++i) {
        auto entry = oam_buffer[i];

        // Need to bounds check the pixel_x here
        Byte pixel_x = entry[1] - 8;
        Quad_Byte pixel_index = (gb.ppu.lcd.line_y * GB_Width) + pixel_x;
        gb.ppu.buffer[pixel_index] = 0xFF000000; // black pixel
    }
    */
}

void ppu_line_state_machine(GameBoy& gb) {
    if(0 <= gb.ppu.lcd.line_y && gb.ppu.lcd.line_y <= 143) {
        ppu_draw_line(gb);

        if(gb.ppu.lcd.line_y == 143) {
            gb.ppu.frame_buffer_callback(gb.ppu.buffer);
            request_vblank_interrupt(gb.interrupt);
        }
    }
    
    if(gb.ppu.lcd.line_y == 153) {
        gb.ppu.lcd.line_y = 0;
    } else {
        gb.ppu.lcd.line_y += 1;
    }
}

void ppu_dot_state_machine(GameBoy& gb) {
    gb.ppu.state.dots_elapsed += 1;
    if(gb.ppu.state.dots_elapsed == 456) {
        // draw line
        ppu_line_state_machine(gb);
        gb.ppu.state.dots_elapsed = 0;
    }
}

namespace PPU {
    Byte read(PPU_Data& ppu, Address addr) {
        if(0xfe00 <= addr && addr <= 0xfe9f) {
            return ppu.oam[addr - 0xfe00];
        }
        else if(addr == 0xff40) {
            return ppu.lcd.control;
        }
        else if(addr == 0xff41) {
            return ppu.lcd.status;
        }
        else if(addr == 0xff42) {
            return ppu.lcd.scroll_y;
        }
        else if(addr == 0xff43) {
            return ppu.lcd.scroll_x;
        }
        else if(addr == 0xff44) {
            return ppu.lcd.line_y;
        }
        else if(addr == 0xff45) {
            return ppu.lcd.line_y_compare;
        }
        else if(addr == 0xff47) {
            return ppu.lcd.background_palette;
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
        else if(addr == 0xff40) {
            ppu.lcd.control = data;
        }
        else if(addr == 0xff41) {
            ppu.lcd.status = data & 0b11111000;
        }
        else if(addr == 0xff42) {
            ppu.lcd.scroll_y = data;
        }
        else if(addr == 0xff43) {
            ppu.lcd.scroll_x = data;
        }
        else if(addr == 0xff44) {
            return;
        }
        else if(addr == 0xff45) {
            ppu.lcd.line_y_compare = data;
        }
        else if(addr == 0xff47) {
            ppu.lcd.background_palette = data;
        }
        else {
            throw std::invalid_argument(std::format(
                "PPU: Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    }
}