#pragma once

#include "data_types.hpp"

inline constexpr auto empty_func = [](auto&) {};

template <typename T, auto AccessFunc = empty_func>
struct Proxy {
    T& obj;
    Address addr;

    Proxy& operator=(Byte data) {
        write(obj, addr, data);

        AccessFunc(obj);

        return *this;
    }

    operator Byte() const {
        Byte data = read(obj, addr);
        
        AccessFunc(obj);

        return data;
    }
};

template <typename T, auto AccessFunc = empty_func>
Proxy<T, AccessFunc> memory_bus(T& obj, Address addr) {
    return {obj, addr};
}
