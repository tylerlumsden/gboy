#pragma once

#include "data_types.hpp"

template <typename T>
struct Proxy {
    T& obj;
    Address addr;

    Proxy& operator=(Byte data) {
        write(obj, addr, data);
        return *this;
    }

    operator Byte() const {
        return read(obj, addr);
    }
};

template <typename T>
Proxy<T> memory_bus(T& obj, Address addr) {
    return {obj, addr};
}
