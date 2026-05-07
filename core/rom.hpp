#pragma once

#include <iostream>
#include <vector>

#include "data_types.hpp"

struct Rom {
    // TODO: data perhaps should be a multiple of std::max<Address>
    std::vector<Byte> data;

    Rom(std::istream& rom_stream);

    void write_as_hex(std::ostream& out);
};