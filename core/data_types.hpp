#pragma once

#include <cstdint>

using Byte = uint8_t;
using Double_Byte = uint16_t;
using Address = uint16_t;

using Signed_Byte = int8_t;
using Signed_Double_Byte = int16_t;

inline Byte lo(Double_Byte data) {
    return (data & 0x0f);
}

inline Byte hi(Double_Byte data) {
    return (data >> 8);
}

inline Double_Byte splice(Byte high, Byte low) {
    return (high << 8) | low;
}