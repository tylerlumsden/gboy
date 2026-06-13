#pragma once

#include <iosfwd>
#include <vector>
#include <format>
#include <bit>

#include "data_types.hpp"

template<Double_Byte Size>
struct WrappingView {
    std::vector<Byte>& data;
    Address offset;

    Byte& operator[](Address addr) const {
        if(addr > Size) {
            throw std::out_of_range(std::format(
                "WrappingView: attemped access to address {:#x}, larger than observed size {:#x}.", 
                addr, Size
            ));
        }
        return data[(addr + offset) % data.size()];
    }
};

struct Rom {
    std::vector<Byte> data;

    std::size_t num_banks;

    Rom(std::vector<Byte> rom_data);
};

struct Ram {
    std::vector<Byte> data;

    std::size_t num_banks;

    Ram(std::size_t num_banks);
};

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