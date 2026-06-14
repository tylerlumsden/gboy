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

size_t rom_bank_size(Byte code) {
    switch(code) {
    case 0x00:
        return 2;
    case 0x01:
        return 4;
    case 0x02:
        return 8;
    case 0x03:
        return 16;
    case 0x04:
        return 32;
    case 0x05:
        return 64;
    case 0x06:
        return 128;
    case 0x07:
        return 256;
    case 0x08:
        return 512;
    case 0x52:
        return 72;
    case 0x53:
        return 80;
    case 0x54:
        return 96;
    }

    throw std::invalid_argument(std::format(
        "Cartridge: rom_bank_size code of {:#x} is invalid.",
        code
    ));
}

size_t ram_bank_size(Byte code) {
    switch(code) {
    case 0x00:
        return 0;
    case 0x01:
        return 0;
    case 0x02:
        return 1;
    case 0x03:
        return 4;
    case 0x04:
        return 16;
    case 0x05:
        return 8;
    }

    throw std::invalid_argument(std::format(
        "Cartridge: ram_bank_size code of {:#x} is invalid.",
        code
    ));
}

template<typename CartridgeType>
void write_func(CartridgeType& cartridge, Address addr, Byte data) {
    if(0x0 <= addr && addr <= 0x1fff) {
        // 0xa is a magic number specifying ram to be enabled
        cartridge.ram_enable = (lo(data) == 0x0a);
    }
    else if(0x2000 <= addr && addr <= 0x3fff) {
        // Masked to five bits (other bits ignored)
        Byte bank_num = data & (0b00011111);

        // We only allow those bits which are within the number of bits required
        // to address our number of rom banks. If we didn't chop off the bits, we could
        // potentially address out-of-range.
        bank_num = chop_least(bank_num, std::bit_width(cartridge.rom.num_banks));
        cartridge.rom_bank_number = bank_num;
    }
    else if(0x4000 <= addr && addr <= 0x5fff) {
        // Masked to two bits (other bits ignored)
        Byte bank_num = data & (0b00000011);
        cartridge.ram_bank_number = bank_num;
    } 
    else {
        throw std::invalid_argument(std::format(
            "Cartridge: Address {:#x} invalid or not implemented for cartridge write.",
            addr
        ));
    }
}

template<typename CartridgeType>
Byte read_func(CartridgeType& cartridge, Address addr) {
    if(0x0 <= addr && addr <= 0x3fff) {
        Quad_Byte resolved_address = addr;
        if(cartridge.bank_mode) {   
            resolved_address = (cartridge.ram_bank_number << 19) | resolved_address;
        }
        return cartridge.rom.data.at(resolved_address);
    }
    else if(0x4000 <= addr && addr <= 0x7fff) {
        // See MBC1 in pandocs for this formula
        Quad_Byte resolved_address = 
        (cartridge.ram_bank_number << 19) | 
        (cartridge.rom_bank_number << 14) | 
        (addr - 0x4000);

        resolved_address = chop_least(resolved_address, 14 + std::bit_width(cartridge.rom.num_banks - 1));

        if(resolved_address == 0x0) resolved_address = 0x1;

        return cartridge.rom.data.at(resolved_address);
    }
    else if(0xa000 <= addr && addr <= 0xbfff) {
        if(cartridge.ram_enable && cartridge.ram.num_banks > 0) {
            Double_Byte resolved_address = (addr - 0xa000);
            if(cartridge.bank_mode && cartridge.ram.num_banks > 1) {
                resolved_address = (cartridge.ram_bank_number << 13) | resolved_address;
            }
            return cartridge.ram.data.at(resolved_address);
        }
        // If ram read access is attempted while disabled, this will trigger (undefined) hardware-specific behaviour.
        // We return 0xff for now as a standard catch-all, but this could be incorrect emulation for some roms.
        return 0xff;
    }
    else {
        throw std::invalid_argument(std::format(
            "Cartridge: Address {:#x} invalid or not implemented for cartridge read",
            addr
        ));
    }
}

namespace CART {

void write(Cartridge& cart, Address addr, Byte data) {
    write_func(cart, addr, data);
}

Byte read(Cartridge& cart, Address addr) {
    return read_func(cart, addr);
}

}

Cartridge construct_cartridge(std::istream&& rom_stream) {
    std::vector<Byte> data = read_data(rom_stream);

    if(data.size() < 0x0150) {
        throw std::invalid_argument(std::format(
            "Cartridge: provided rom size of {:#x} is less than minimum {:#x}",
            data.size(), 0x0150
        ));
    }

    Byte cartridge_type = data[0x0147];
    Byte rom_bank_code = data[0x0148];
    Byte ram_bank_code = data[0x0149];
    switch(cartridge_type) {

    case 0x01: {
        return MBC1Cartridge{
            Rom(data, rom_bank_size(rom_bank_code)), Ram(ram_bank_size(ram_bank_code))
        };
    }

    default:
        throw std::logic_error(std::format(
            "Cartridge: provided rom has invalid or unimplemented cartridge type {:#x}",
            cartridge_type
        ));
    }
}

Ram::Ram(size_t num_banks) : num_banks(num_banks) {
    data.resize(0x2000 * num_banks);
}

Rom::Rom(std::vector<Byte> rom_data, std::size_t num_banks) : data(rom_data), num_banks(num_banks) {}
