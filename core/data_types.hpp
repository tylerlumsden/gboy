#pragma once

#include <concepts>
#include <cstdint>
#include <array>

using Bit = bool;

using Byte = uint8_t;
using Double_Byte = uint16_t;
using Address = uint16_t;

using Signed_Byte = int8_t;
using Signed_Double_Byte = int16_t;

template <std::unsigned_integral T>
inline auto lo(T data) {
    data = static_cast<T>(data << 4 * sizeof(T));
    return (data >> 4 * sizeof(T));
}

template <std::unsigned_integral T>
inline auto hi(T data) {
    return (data >> 4 * sizeof(T));
}

template <std::unsigned_integral T>
constexpr auto bit_array(T data) {
    constexpr std::size_t num_bits = 8 * sizeof(T);
    std::array<Bit, num_bits> array;

    for(std::size_t i = 0; i < num_bits; ++i) {
        array[i] = (data & 0x1);
        data = (data >> 1);
    }

    return array;
}

template <std::same_as<Byte> T>
inline Double_Byte splice(T high, T low) {
    return (high << 8) | low;
}
