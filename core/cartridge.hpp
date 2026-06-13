#pragma once

#include <iosfwd>
#include <vector>
#include <format>
#include <bit>

#include "data_types.hpp"

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
struct Cartridge {
    MBC1Cartridge impl;

    Byte read(Address addr);
    void write(Address addr, Byte data);
};

Cartridge construct_cartridge(std::istream& rom_stream);