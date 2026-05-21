#include <iostream>

#include "rom.hpp"

std::vector<Byte> read_data(std::istream& stream) {
    std::vector<Byte> data_vec;

    char data;
    while(stream.get(data)) {
        data_vec.push_back(static_cast<Byte>(data));
    }
    return data_vec;
}

Rom::Rom(std::istream& rom_stream) : 
    data(read_data(rom_stream)), 
    fixed_bank{data, 0}, 
    slotted_bank{data, 0x4000}  {}

void Rom::write_as_hex(std::ostream& out) {
    constexpr size_t newline_step = 1;
    for(size_t i = 0; i < this->data.size(); ++i) {
        if(i % newline_step == 0 && i != 0) {
            out << "\n";
        }
        out << std::hex << i << " " << static_cast<int>(this->data[i]) << " ";
    }
}