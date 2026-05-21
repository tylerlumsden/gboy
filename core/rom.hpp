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
    std::vector<Byte> data;
    const WrappingView<0x4000> fixed_bank;
    WrappingView<0x4000> slotted_bank;

    bool external_ram_toggle = false;

    Rom(std::istream& rom_stream);

    void write_as_hex(std::ostream& out);
};