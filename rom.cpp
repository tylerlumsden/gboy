#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using Byte = uint8_t;

struct Rom {
    std::vector<Byte> rom_data;

    Rom(std::istream& rom_stream);

    void write_as_hex(std::ostream& out);
};

Rom::Rom(std::istream& rom_stream) {
    char data;
    while(rom_stream.get(data)) {
        this->rom_data.push_back(static_cast<Byte>(data));
    }
}

void Rom::write_as_hex(std::ostream& out) {
    for(size_t i = 0; i < rom_data.size(); ++i) {
        if(i % 8 == 0 && i != 0) {
            out << "\n";
        }
        out << std::hex << static_cast<int>(rom_data[i]) << " ";
    }
}

int main() {
    std::ifstream reader("pkmnblue.gb");
    Rom pkmn_data(reader);

    std::ofstream writer("pkmnblue.dat");
    pkmn_data.write_as_hex(writer);
}