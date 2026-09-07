#pragma once
#include <array>
#include <functional>

#include "data_types.hpp"

inline constexpr Byte GB_Width = 160;
inline constexpr Byte GB_Height = 144;

using FrameBuffer = std::array<Quad_Byte, GB_Width * GB_Height>;
using FrameBufferCallback = std::function<void(FrameBuffer&)>;