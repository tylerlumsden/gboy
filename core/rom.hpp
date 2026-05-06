#pragma once

#include <iostream>
#include <vector>
#include <cstdint>

using Byte = uint8_t;
struct Rom {
    std::vector<Byte> rom_data;

    Rom(std::istream& rom_stream);

    void write_as_hex(std::ostream& out);
};