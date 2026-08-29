#pragma once

#include <format>

#include "data_types.hpp"

namespace GB { struct GameBoy; }

struct Timer {
    Quad_Byte cycles_per_second = 256;
    Double_Byte system_counter = 0xab00;
    Byte counter = 0;
    Byte modulo = 0;
    Byte control = 0xf8;
    bool overflow_flag = false;
    bool overflow_latch = false;
};

template <>
struct std::formatter<Timer> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Timer& timer, std::format_context& ctx) const {
        return std::format_to(ctx.out(), R"(
            Timer State:
            cycles_per_second: {}
            system_counter: {:#x}
            counter: {:#x}
            modulo: {:#x}
            control: {:#x}
            overflow: {}
            )",
            timer.cycles_per_second, timer.system_counter, timer.counter,
            timer.modulo, timer.control, timer.overflow_flag
        );
    }
};

void m_cycle_tick(GB::GameBoy& gb);
void m_cycle_tick(GB::GameBoy& gb, Byte count);

namespace TIMER {
    Byte read(Timer& timer, Address addr);
    void write(Timer& timer, Address addr, Byte data);
}