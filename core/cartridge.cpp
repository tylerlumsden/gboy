#include <iostream>
#include <stdexcept>
#include <format>

#include "cartridge.hpp"

std::vector<Byte> read_data(std::istream& stream) {
    std::vector<Byte> data_vec;

    char data;
    while(stream.get(data)) {
        data_vec.push_back(static_cast<Byte>(data));
    }
    return data_vec;
}

template<typename CartridgeType>
void write_func(CartridgeType& cartridge, Address addr, Byte data) {
    throw std::invalid_argument(std::format(
        "Cartridge: write_func not implemented."
    ));
}

template<typename CartridgeType>
Byte read_func(CartridgeType& cartridge, Address addr) {
    throw std::invalid_argument(std::format(
        "Cartridge: read_func not implemented."
    ));
}

Cartridge construct_cartridge(std::istream& rom_stream) {
    std::vector<Byte> data = read_data(rom_stream);

    if(data.size() < 0x0150) {
        throw std::invalid_argument(std::format(
            "Cartridge: provided rom size of {:#x} is less than minimum {:#x}",
            data.size(), 0x0150
        ));
    }

    Byte cartridge_type = data[0x0147];
    switch(cartridge_type) {

    case 0x01: {
        return Cartridge{.impl = MBC1Cartridge{Rom(data), Ram()}};
    }

    default:
        throw std::logic_error(std::format(
            "Cartridge: provided rom has invalid or unimplemented cartridge type {:#x}",
            cartridge_type
        ));
    }
}

Rom::Rom(std::vector<Byte> rom_data) : data(rom_data), 
    fixed_bank{data, 0}, 
    slotted_bank{data, 0x4000}  {
        
    Byte bank_exponent = data[0x0148];
    if(!(0x0 <= bank_exponent && bank_exponent <= 0x5)) {
        throw std::invalid_argument(std::format(
            "Rom: bank exponent has invalid value {}",
            bank_exponent
        ));
    }
    this->bank_bits = bank_exponent;
}

/*
void Rom::write_as_hex(std::ostream& out) {
    constexpr size_t newline_step = 1;
    for(size_t i = 0; i < this->data.size(); ++i) {
        if(i % newline_step == 0 && i != 0) {
            out << "\n";
        }
        out << std::hex << i << " " << static_cast<int>(this->data[i]) << " ";
    }
}
*/