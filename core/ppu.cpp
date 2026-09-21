#include "gameboy.hpp"
#include "ppu.hpp"
#include "memory.hpp"

#include "log.hpp"

#include <tuple>
#include <algorithm>

using GB::GameBoy;

Double_Byte resolve_line_y(Quad_Byte dots_elapsed) {
    return dots_elapsed / 456;
}

Double_Byte resolve_line_dots(Quad_Byte dots_elapsed) {
    return dots_elapsed % 456;
}

Byte pixel_data_to_color_id(Byte low_data, Byte high_data, Byte pixel_index) {
    pixel_index = 7 - pixel_index;
    Byte color_id = ((high_data >> pixel_index) & 0x1) << 1 | ((low_data >> pixel_index) & 0x1);
    return color_id;
}

Quad_Byte color_map(Byte palette, Byte color_id) {
    Byte shade;
    switch(color_id) {
    case 0:
        shade = (palette & 0b11);
        break;
    case 1:
        shade = ((palette >> 2) & 0b11);
        break;
    case 2:
        shade = ((palette >> 4) & 0b11);
        break;
    case 3:
        shade = ((palette >> 6) & 0b11);
        break;
    default:
        throw std::invalid_argument(
            std::format("color_map: invalid color_id provided: {}", color_id)
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

void debug_render_tileset(GameBoy& gb, TilemapBuffer& buffer) {

    for(Address index = 0x0; index <= 0x17ff; index += 16) {
        for(int line = 0; line < 8; ++line) {
            Address line_addr = 0x8000 + index + (line * 2);
            Byte low_data = memory_bus(gb, line_addr);
            Byte high_data = memory_bus(gb, line_addr + 1);
            for(int pixel = 7; pixel >= 0; --pixel) {
                Byte color_id = pixel_data_to_color_id(low_data, high_data, pixel);
                Quad_Byte color = color_map(gb.ppu.lcd.background_palette, color_id);

                Quad_Byte tile_id = index / 16;

                // index % 16 is the tile coordinate x * 8 is the pixel coordinate x
                Quad_Byte coordinate_x = ((tile_id % 32) * 8) + pixel;
                // (index / (16 * 32)) is the tile coordinate y + line is the pixel coordinate y
                Quad_Byte coordinate_y = ((tile_id / 32) * 8) + line;

                buffer[(coordinate_y * Tilemap_Width) + coordinate_x] = color;
            }   
        }
    }
}

std::pair<std::array<OAM_Entry, 10>, Byte> oam_scan(std::array<Byte, 0xa0> oam, Double_Byte line_y, Byte tile_height) {
    // Loop through all entries in the OAM and record all collisions on the current line

    std::array<OAM_Entry, 10> oam_buffer;
    Byte oam_list_size = 0;

    for(Address offset = 0; offset < 160 && oam_list_size < 10; offset += 4) {
        // As per pandocs, the position is the actual position + 16
        Byte oam_position_y = oam[offset];

        Byte position_y = oam_position_y - 16;

        if(position_y <= line_y && line_y < position_y + tile_height) {
            Byte oam_position_x = oam[offset + 1];
            Byte oam_tile_index = oam[offset + 2];
            Byte oam_attribute = oam[offset + 3];

            oam_buffer[oam_list_size] = {oam_position_y, oam_position_x, oam_tile_index, oam_attribute};
            ++oam_list_size;
        }
    }

    return {oam_buffer, oam_list_size};
}

using Color_Line = std::array<Byte, GB_Width>;
using Priority_List = std::array<bool, GB_Width>;
using Palette_List = std::array<Byte, GB_Width>;
std::tuple<Color_Line, Priority_List, Palette_List> draw_oam_line(GameBoy& gb) {
    Color_Line line{};
    Priority_List priority{};
    Palette_List palettes{};

    Byte tile_height = 8;
    if(get_bit(gb.ppu.lcd.control, 2)) {
        tile_height = 16;
    }
    auto [oam_buffer, oam_list_size] = oam_scan(gb.ppu.oam, gb.ppu.lcd.line_y, tile_height);

    std::stable_sort(oam_buffer.begin(), oam_buffer.begin() + oam_list_size, 
    [](const OAM_Entry& a, const OAM_Entry& b) {
        return a[1] < b[1];
    });

    std::reverse(oam_buffer.begin(), oam_buffer.begin() + oam_list_size);


    for(int i = 0; i < oam_list_size; ++i) {
        auto entry = oam_buffer[i];

        Byte position_x = entry[1] - 8;
        Byte position_y = entry[0] - 16; 

        bool y_flip = get_bit(entry[3], 6);
        Byte tile_row = gb.ppu.lcd.line_y - position_y;
        if(y_flip) {
            tile_row = tile_height - tile_row - 1;
        }

        Address tile_index;
        if(get_bit(gb.ppu.lcd.control, 2)) {
            tile_index = (entry[2] & ~1) * 16;
        } else {
            tile_index = entry[2] * 16;
        }

        Address tile_addr = 0x8000 + tile_index + (tile_row * 2);
        
        Byte low_data = memory_bus(gb, tile_addr);
        Byte high_data = memory_bus(gb, tile_addr + 1);

        bool x_flip = get_bit(entry[3], 5);
        bool palette_mode = get_bit(entry[3], 4);
        Byte palette = gb.ppu.lcd.object_palette1;
        if(palette_mode) {
            palette = gb.ppu.lcd.object_palette2;
        }

        for(int pixel = 0; pixel < 8; ++pixel) {
            Quad_Byte horizontal_index = position_x + pixel;
            if(x_flip) {
                horizontal_index = position_x + (8 - pixel - 1);
            }

            Byte color_id = pixel_data_to_color_id(low_data, high_data, pixel);
            if(color_id != 0 && 0 <= horizontal_index && horizontal_index <= GB_Width) {
                line[horizontal_index] = color_id;
                priority[horizontal_index] = get_bit(entry[3], 7);
                palettes[horizontal_index] = palette;
            }
        }
    }

    return {line, priority, palettes};
}

Color_Line draw_background_line(GameBoy& gb) {
    Color_Line line;

    auto lcdc = bit_array(gb.ppu.lcd.control);
    Address map_offset = 0x9800;
    if(lcdc[3]) {
        map_offset = 0x9c00;
    }

    Byte line_y = (gb.ppu.lcd.line_y + gb.ppu.lcd.scroll_y) % 256;
    Byte tile_row = (line_y % 8) * 2;
    Double_Byte map_row = (line_y / 8) * 32;

    Byte line_x = gb.ppu.lcd.scroll_x;
    int render_x = -(line_x % 8);

    while(render_x < 160) {
        Address map_address = map_offset + map_row + (line_x / 8);
        Byte tile_index = memory_bus(gb, map_address);

        Address tile_address;
        if(lcdc[4]) {
            tile_address = 0x8000 + (tile_index * 16) + tile_row;
        } else {
            tile_address = 0x9000 + static_cast<Signed_Byte>(tile_index) * 16 + tile_row;
        }

        Byte low_data = memory_bus(gb, tile_address);
        Byte high_data = memory_bus(gb, tile_address + 1);

        for(int pixel = 0; pixel < 8; ++pixel) {
            if(0 <= render_x && render_x < 160) {
                Byte color_id = pixel_data_to_color_id(low_data, high_data, pixel);
                line[render_x] = color_id;
            }
            render_x += 1;
        }
        line_x += 8;
    }
    return line;
}

void ppu_draw_line(GameBoy& gb) {

    // Initializes all elements to color id zero
    Color_Line bg_line{};
    if(get_bit(gb.ppu.lcd.control, 0)) {
        bg_line = draw_background_line(gb);
    }

    // Initializes all elements to color id zero (transparent for obj)
    Color_Line obj_line{};
    Priority_List obj_priority{};
    Palette_List obj_palettes{};
    if(get_bit(gb.ppu.lcd.control, 1)) {
        auto [line, priority, palettes] = draw_oam_line(gb);  
        obj_line = line;
        obj_priority = priority;
        obj_palettes = palettes;
    }   

    for(size_t i = 0; i < GB_Width; ++i) {
        Quad_Byte color;

        bool obj_has_priority = !(obj_priority[i] && bg_line[i] != 0); 
        bool obj_not_transparent = (obj_line[i] != 0);
        if(obj_not_transparent && obj_has_priority) {
            color = color_map(obj_palettes[i], obj_line[i]);
        } else {
            color = color_map(gb.ppu.lcd.background_palette, bg_line[i]);
        }

        gb.ppu.buffer[gb.ppu.lcd.line_y * GB_Width + i] = color;
    }
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
    if(gb.ppu.state.dots_elapsed == 456) {
        // draw line
        ppu_line_state_machine(gb);

        gb.ppu.state.dots_elapsed = 0;
    } else {
        gb.ppu.state.dots_elapsed += 1;
    }

    if(!get_bit(gb.ppu.lcd.status, 2) && gb.ppu.lcd.line_y == gb.ppu.lcd.line_y_compare) {
        gb.ppu.lcd.status = set_bit(gb.ppu.lcd.status, 2);
        request_lcd_interrupt(gb.interrupt);
    } else if(gb.ppu.lcd.line_y != gb.ppu.lcd.line_y_compare) {
        gb.ppu.lcd.status = clear_bit(gb.ppu.lcd.status, 2);
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
        else if(addr == 0xff48) {
            return ppu.lcd.object_palette1;
        }
        else if(addr == 0xff49) {
            return ppu.lcd.object_palette2;
        }
        else {
            return 0;
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
            ppu.lcd.status = (data & 0b11111000) | (ppu.lcd.status & 0b00000111);
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
        else if(addr == 0xff48) {
            ppu.lcd.object_palette1 = data;
        }
        else if(addr == 0xff49) {
            ppu.lcd.object_palette2 = data;
        }
        else {
            return;
            throw std::invalid_argument(std::format(
                "PPU: Attempted to write to address {:#x}. This address is either unimplemented or out of range.\n", addr
            ));
        }
    }
}