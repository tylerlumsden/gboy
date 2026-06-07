#pragma once

#include <iosfwd>
#include <vector>
#include <format>

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
    // We use a wrapping view here defensively.
    // This is because the Rom is data loaded externally from the program.
    // Hence if the program were provided Rom data that is not the expected size,
    // we mimic the Gameboy's behaviour by wrapping the memory instead.
    Byte bank_bits;

    std::vector<Byte> data;
    const WrappingView<0x4000> fixed_bank;
    WrappingView<0x4000> slotted_bank;

    void set_bank_id(Byte bank_id) {
        bank_id = (bank_id & 0b00011111);
        if(bank_id == 0x0) bank_id = 0x1;

        this->slotted_bank.offset = bank_id * 0x4000;
    }

    void write_as_hex(std::ostream& out);

    Rom(std::istream& rom_stream);
};

struct Ram {
    std::array<Byte, 0x8000> data;
    std::span<Byte, 0x2000> slotted_bank;

    Ram() : slotted_bank(data.data(), 0x2000) {}
};

struct RawCartridge {
    Rom rom;
    Ram ram;
};

struct MBC1Cartridge {
    RawCartridge data;
};

// Generic interface for the cartridge implementation.
// In the future we may have several types of cartridges,
// so this unifies them under the same interface.
struct Cartridge {
    RawCartridge impl;
};

Cartridge construct_cartridge(std::istream& rom_stream);