#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <bit>

#include "data_types.hpp"

inline void write_as_hex(std::vector<Byte> data, std::ostream&& out) {
    constexpr size_t newline_step = 1;
    for(size_t i = 0; i < data.size(); ++i) {
        if(i % newline_step == 0 && i != 0) {
            out << "\n";
        }
        out << std::hex << i << " " << static_cast<int>(data[i]) << " ";
    }
}

struct Rom {
    std::vector<Byte> data;
    
    // Stored here for convenience
    std::size_t num_banks;

    Rom(std::vector<Byte> rom_data, std::size_t num_banks);
};

struct Ram {
    std::vector<Byte> data;

    // Stored here for convenience
    std::size_t num_banks;

    Ram(std::size_t num_banks);
};

// TODO: Template this with HasRam, HasBattery and concept checks in the read/write
struct MBC1Cartridge {
    Rom rom;
    Ram ram;

    Byte rom_bank_number = 0x0;
    Byte ram_bank_number = 0x0;
    bool bank_mode = 0x0; 

    bool ram_enable = 0x0;
};

// Generic interface for the cartridge implementation.
// In the future we may have several types of cartridges,
// so this unifies them under the same interface.
using Cartridge = MBC1Cartridge;

namespace CART {
void write(Cartridge& cart, Address addr, Byte data);
Byte read(Cartridge& cart, Address addr);
}

Cartridge construct_cartridge(std::istream&& rom_stream);