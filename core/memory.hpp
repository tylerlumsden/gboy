#pragma once

#include <functional>
#include "data_types.hpp"

struct MemoryBus {
    std::function<void(Address, Byte)> write;
    std::function<Byte(Address)> read;

    struct Proxy {
        MemoryBus& bus;
        Address addr;

        Proxy& operator=(Byte data) {
            bus.write(addr, data);
            return *this;
        }

        operator Byte() const {
            return bus.read(addr);
        }
    };

    Proxy operator[](Address addr) {
        return Proxy{*this, addr};
    }
};