#pragma once

#include <functional>
#include "data_types.hpp"

struct MemoryBus {
    std::function<Byte&(Address)> address_func;

    Byte& operator[](Address addr) {
        return address_func(addr);
    }
};