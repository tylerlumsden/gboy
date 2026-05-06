#include <fstream>

#include "rom.hpp"

Rom::Rom(std::istream& rom_stream) {
    char data;
    while(rom_stream.get(data)) {
        this->rom_data.push_back(static_cast<Byte>(data));
    }
}

void Rom::write_as_hex(std::ostream& out) {
    constexpr size_t newline_step = 1;
    for(size_t i = 0; i < rom_data.size(); ++i) {
        if(i % newline_step == 0 && i != 0) {
            out << "\n";
        }
        out << std::hex << i << " " << static_cast<int>(rom_data[i]) << " ";
    }
}