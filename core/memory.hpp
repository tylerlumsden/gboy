#pragma once

#include <concepts>

#include "data_types.hpp"

struct empty_func {
    void operator()() const noexcept {}
};

template <typename T, typename AccessFunc = empty_func>
struct Proxy {
    T& obj;
    Address addr;
    AccessFunc access;

    Proxy& operator=(Byte data) {
        access();

        write(obj, addr, data);

        return *this;
    }

    operator Byte() const {
        access();

        Byte data = read(obj, addr);

        return data;
    }
};

template <typename T, typename AccessFunc = empty_func>
Proxy<T, AccessFunc> memory_bus(T& obj, Address addr, AccessFunc access = {}) {
    return {obj, addr, access};
}
