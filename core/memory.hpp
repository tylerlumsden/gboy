#pragma once

#include <functional>
#include "data_types.hpp"

struct MemoryBus {
    std::function<void(Address, Byte)> write;
    std::function<const Byte&(Address)> read;
};