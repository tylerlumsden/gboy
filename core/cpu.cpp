#include <cmath>
#include <utility>
#include <array>
#include <stdexcept>
#include <format>
#include <functional>
#include <bitset>

#include "gameboy.hpp"
#include "memory.hpp"
#include "data_types.hpp"
#include "log.hpp"

using GB::GameBoy;

namespace {

// --- Utilities ---

template <std::convertible_to<Byte>... Operands>
bool half_carry_add(Byte a, Operands... operands) {
    std::array<Byte, sizeof...(operands)> operand_array{operands...};
    Byte running_sum = a;
    for(Byte operand : operand_array) {
        if((running_sum & 0x0f) + (operand & 0x0f) > 0x0f) {
            return true;
        }
        running_sum += operand;
    }
    return false;
}

template <std::convertible_to<Byte>... Operands>
bool carry_add(Byte a, Operands... operands) {
    std::array<Byte, sizeof...(operands)> operand_array{operands...};
    Byte running_sum = a;
    for(Byte operand : operand_array) {
        if(static_cast<Byte>(running_sum + operand) < running_sum) {
            return true;
        }
        running_sum += operand;
    }
    return false;
}

template <std::convertible_to<Byte>... Operands>
bool half_carry_sub(Byte a, Operands... operands) {
    std::array<Byte, sizeof...(operands)> operand_array{operands...};
    Byte running_sum = a;
    for(Byte operand : operand_array) {
        if((running_sum & 0x0f) < (operand & 0x0f)) {
            return true;
        }
        running_sum -= operand;
    }
    return false;
}

template <std::convertible_to<Byte>... Operands>
bool carry_sub(Byte a, Operands... operands) {
    std::array<Byte, sizeof...(operands)> operand_array{operands...};
    Byte running_sum = a;
    for(Byte operand : operand_array) {
        if(running_sum < operand) {
            return true;
        }
        running_sum -= operand;
    }
    return false;
}

Byte flags_as_byte(GameBoy& gb) {
    Byte flags = 0x0;
    flags |= (gb.processor.carry_flag << 4);
    flags |= (gb.processor.half_carry_flag << 5);
    flags |= (gb.processor.subtraction_flag << 6);
    flags |= (gb.processor.zero_flag << 7);

    return flags;
}

void byte_as_flags(GameBoy& gb, Byte data) {
    gb.processor.carry_flag = (data & 0b00010000);
    gb.processor.half_carry_flag = (data & 0b00100000);
    gb.processor.subtraction_flag = (data & 0b01000000);
    gb.processor.zero_flag = (data & 0b10000000);
}

auto cpu_memory_bus(GameBoy& gb, Address addr) {
    return memory_bus(gb, addr, [&gb]() { m_cycle_tick(gb); });
}

Byte fetch(GameBoy& gb) {

    Byte retval = cpu_memory_bus(gb, gb.processor.program_counter);
    ++gb.processor.program_counter;

    Log::log<Log::Level::Verbose>("Fetched byte {:#x}", retval);    
    return retval;
}

Double_Byte fetch_double(GameBoy& gb) {
    Byte low = fetch(gb);
    Byte high = fetch(gb);
    
    return splice(high, low);
}

void push(GameBoy& gb, Double_Byte data) {
    Byte high = hi(data);
    Byte low = lo(data);

    m_cycle_tick(gb);

    --gb.processor.stack_pointer;
    cpu_memory_bus(gb, gb.processor.stack_pointer) = high;

    --gb.processor.stack_pointer;
    cpu_memory_bus(gb, gb.processor.stack_pointer) = low;
}

Double_Byte pop(GameBoy& gb) {
    Byte low = cpu_memory_bus(gb, gb.processor.stack_pointer);
    ++gb.processor.stack_pointer;

    Byte high = cpu_memory_bus(gb, gb.processor.stack_pointer);
    ++gb.processor.stack_pointer;

    return splice(high, low);
}

// --- Instructions ---

template<Byte Opcode>
void no_impl(GameBoy& gb) {
    throw std::logic_error(std::format("Opcode with byte value {:#x} is not implemented.\n", Opcode));
}

template<Byte Opcode>
void cb_no_impl(GameBoy& gb) {
    throw std::logic_error(std::format("cb-prefixed Opcode with byte value {:#x} is not implemented.\n", Opcode));
}

void nop(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing nop {:#x}", 0x00);
    // Should use 4 cycles
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xe9, 0xca, 0xda>
void jp(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing jp {:#x}", Opcode);

    Address jump_addr = [&]() {
        if constexpr(is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xca, 0xda>) return fetch_double(gb);
        else if constexpr(Opcode == 0xe9) return splice(gb.processor.H, gb.processor.L);
    }();

    if constexpr(Opcode == 0xc2) {
        if(gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0xd2) {
        if(gb.processor.carry_flag) return;
    }
    else if constexpr(is_one_of<Opcode, 0xc3, 0xe9>) {
        // Conditionless jump
    }
    else if constexpr(Opcode == 0xca) {
        if(!gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0xda) {
        if(!gb.processor.carry_flag) return;
    }

    if constexpr(Opcode != 0xe9) {
        m_cycle_tick(gb);
    }
    gb.processor.program_counter = jump_addr;
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0x20, 0x30, 0x18, 0x28, 0x38>
void jr(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing jr {:#x}", Opcode);
    Signed_Byte relative_address = fetch(gb);
    if constexpr(Opcode == 0x20) {
        if(gb.processor.zero_flag) return;
    }
    // Opcode 0x18 has no condition check, including constexpr for consistency
    else if constexpr(Opcode == 0x18);
    else if constexpr(Opcode == 0x28) {
        if(!gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0x30) {
        if(gb.processor.carry_flag) return;
    } 
    else if constexpr(Opcode == 0x38) {
        if(!gb.processor.carry_flag) return;
    }

    m_cycle_tick(gb);
    gb.processor.program_counter += relative_address;
}

// TODO: implement variants
template<Byte Opcode>
void x_or(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing x_or {:#x}", Opcode);
    if constexpr(Opcode == 0xaf) {
        gb.processor.A = gb.processor.A ^ gb.processor.A;
    }
}

void di(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing di {:#x}", 0xf3);
    gb.processor.IME = false;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x02, 0x0a, 0x12, 0x1a, 0x22, 0x2a, 0x32, 0x3a>
void ld_address_a(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_address_a {:#x}", Opcode);
    constexpr Byte addr_type = (Opcode & 0xf0);
    Address address = [&]() {
        if constexpr(addr_type == 0x00) return splice(gb.processor.B, gb.processor.C);
        else if constexpr(addr_type == 0x10) return splice(gb.processor.D, gb.processor.E);
        else if constexpr(addr_type == 0x20) {
            Address addr = splice(gb.processor.H, gb.processor.L);
            Address addr_increment = addr + 1;
            gb.processor.H = hi(addr_increment);
            gb.processor.L = lo(addr_increment);
            return addr;
        }
        else if constexpr(addr_type == 0x30) {
            Address addr = splice(gb.processor.H, gb.processor.L);
            Address addr_decrement = addr - 1;
            gb.processor.H = hi(addr_decrement);
            gb.processor.L = lo(addr_decrement);
            return addr;
        }
    }();

    constexpr Byte load_order = (Opcode & 0x0f);
    if constexpr(load_order == 0x02) {
        cpu_memory_bus(gb, address) = gb.processor.A;
    }
    else if constexpr(load_order == 0x0a) {
        gb.processor.A = cpu_memory_bus(gb, address);
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xe0, 0xe2, 0xea, 0xf0, 0xf2, 0xfa>
void ld_address_a_misc(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_address_a_misc {:#x}", Opcode);
    constexpr Byte addr_type = (Opcode & 0x0f);
    Address address = [&]() {
        if constexpr(addr_type == 0x00) {
            Byte addr = fetch(gb);
            return 0xff00 | addr;
        }
        else if constexpr(addr_type == 0x02) {
            return 0xff00 | gb.processor.C;
        }
        else if constexpr(addr_type == 0x0a) {
            Byte low_byte = fetch(gb);
            Byte high_byte = fetch(gb);

            return splice(high_byte, low_byte);
        }
    }();

    constexpr Byte load_order = (Opcode & 0xf0);
    if constexpr(load_order == 0xe0) {
        cpu_memory_bus(gb, address) = gb.processor.A;
    }
    else if constexpr(load_order == 0xf0) {
        gb.processor.A = cpu_memory_bus(gb, address);
    }
}



template<Byte Opcode>
void ld_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_register {:#x}", Opcode);
    decltype(auto) register_mapping = [&]<Byte Regcode>() -> decltype(auto) {
        if constexpr(Regcode == 0b000) return static_cast<Byte&>(gb.processor.B);
        else if constexpr(Regcode == 0b001) return static_cast<Byte&>(gb.processor.C);
        else if constexpr(Regcode == 0b010) return static_cast<Byte&>(gb.processor.D);
        else if constexpr(Regcode == 0b011) return static_cast<Byte&>(gb.processor.E);
        else if constexpr(Regcode == 0b100) return static_cast<Byte&>(gb.processor.H);
        else if constexpr(Regcode == 0b101) return static_cast<Byte&>(gb.processor.L);
        else if constexpr(Regcode == 0b110) return cpu_memory_bus(gb, splice(gb.processor.H, gb.processor.L));
        else if constexpr(Regcode == 0b111) return static_cast<Byte&>(gb.processor.A);
    };

    constexpr Byte DestCode = (Opcode & 0b00111000) >> 3;
    constexpr Byte SourceCode = (Opcode & 0b00000111);

    register_mapping.template operator()<DestCode>() = register_mapping.template operator()<SourceCode>();
}

template<Byte Opcode>
    requires (Opcode == 0x08)
void ld_a16_sp(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_a16_sp {:#x}", Opcode);
    Byte low_byte = fetch(gb);
    Byte high_byte = fetch(gb);
    Address addr = splice(high_byte, low_byte);

    cpu_memory_bus(gb, addr) = lo(gb.processor.stack_pointer);
    cpu_memory_bus(gb, addr + 1) = hi(gb.processor.stack_pointer);
}


template<Byte Opcode>
    requires (Opcode == 0xf9)
void ld_sp_hl(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_sp_hl {:#x}", Opcode);
    gb.processor.stack_pointer = splice(gb.processor.H, gb.processor.L);
    m_cycle_tick(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x06, 0x0e, 0x16, 0x1e, 0x26, 0x2e, 0x36, 0x3e>
void ld_n8(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_n8 {:#x}", Opcode);
    decltype(auto) load_dest = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x06) return static_cast<Byte&>(gb.processor.B);
        if constexpr(Opcode == 0x0e) return static_cast<Byte&>(gb.processor.C);
        if constexpr(Opcode == 0x16) return static_cast<Byte&>(gb.processor.D);
        if constexpr(Opcode == 0x1e) return static_cast<Byte&>(gb.processor.E);
        if constexpr(Opcode == 0x26) return static_cast<Byte&>(gb.processor.H);
        if constexpr(Opcode == 0x2e) return static_cast<Byte&>(gb.processor.L);
        if constexpr(Opcode == 0x36) return cpu_memory_bus(gb, splice(gb.processor.H, gb.processor.L));
        if constexpr(Opcode == 0x3e) return static_cast<Byte&>(gb.processor.A);
    };

    load_dest() = fetch(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x01, 0x11, 0x21, 0x31>
void ld_n16(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_n16 {:#x}", Opcode);
    Byte low = fetch(gb);
    Byte high = fetch(gb);

    if constexpr(Opcode == 0x31) {
        gb.processor.stack_pointer = splice(high, low);
    } else {
        auto [high_byte, low_byte] = [&]() -> std::pair<Byte&, Byte&> {
            if constexpr(Opcode == 0x01) return {gb.processor.B, gb.processor.C};
            else if constexpr(Opcode == 0x11) return {gb.processor.D, gb.processor.E};
            else if constexpr(Opcode == 0x21) return {gb.processor.H, gb.processor.L};
        }();

        high_byte = high; 
        low_byte = low;
    }
}

// TODO: implement variants
template<Byte Opcode>
void rst(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing rst {:#x}", Opcode);
    Byte high_byte = hi(gb.processor.program_counter);
    Byte low_byte = lo(gb.processor.program_counter);
    auto high_memory = cpu_memory_bus(gb, gb.processor.stack_pointer);
    high_memory = high_byte;
    --gb.processor.stack_pointer;

    auto low_memory = cpu_memory_bus(gb, gb.processor.stack_pointer);
    low_memory = low_byte;
    --gb.processor.stack_pointer;


    m_cycle_tick(gb);
    if constexpr(Opcode == 0xff) {
        gb.processor.program_counter = 0x38;
    }
}

// TODO: implement variants
template<Byte Opcode>
void inc(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing inc {:#x}", Opcode);
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x3c) return gb.processor.A;
    }();

    ++data;
}

// TODO: implement variants
template<Byte Opcode>
void ldh(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ldh {:#x}", Opcode);
    if constexpr(Opcode == 0xe0) {
        Byte low_byte = fetch(gb);
        Byte high_byte = 0xff;

        Address addr = splice(high_byte, low_byte);

        write(gb, addr, gb.processor.A);
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc4, 0xd4, 0xcc, 0xdc, 0xcd>
void call(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing call {:#x}", Opcode);
    Byte low_byte = fetch(gb);
    Byte high_byte = fetch(gb);
    if constexpr(Opcode == 0xc4) {
        if(gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0xd4) {
        if(gb.processor.carry_flag) return;
    }
    else if constexpr(Opcode == 0xcc) {
        if(!gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0xdc) {
        if(!gb.processor.carry_flag) return;
    }

    Address subroutine = splice(high_byte, low_byte);

    push(gb, gb.processor.program_counter);
    gb.processor.program_counter = subroutine;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc5, 0xd5, 0xe5, 0xf5>
void push_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing push_register {:#x}", Opcode);
    Double_Byte push_data = [&]() {
        if constexpr(Opcode == 0xc5) return splice(gb.processor.B, gb.processor.C);
        else if constexpr(Opcode == 0xd5) return splice(gb.processor.D, gb.processor.E);
        else if constexpr(Opcode == 0xe5) return splice(gb.processor.H, gb.processor.L);
        else if constexpr(Opcode == 0xf5) return splice(gb.processor.A, flags_as_byte(gb));
    }();

    push(gb, push_data);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc1, 0xd1, 0xe1, 0xf1>
void pop_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing pop_register {:#x}", Opcode);
    Double_Byte data = pop(gb);

    if constexpr(Opcode == 0xc1) {
        gb.processor.B = hi(data);
        gb.processor.C = lo(data);
    }
    else if constexpr(Opcode == 0xd1) {
        gb.processor.D = hi(data);
        gb.processor.E = lo(data);
    }
    else if constexpr(Opcode == 0xe1) {
        gb.processor.H = hi(data);
        gb.processor.L = lo(data);
    }
    else if constexpr(Opcode == 0xf1) {
        gb.processor.A = hi(data);
        byte_as_flags(gb, lo(data));
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x03, 0x13, 0x23, 0x0b, 0x1b, 0x2b>
void inc_dec_double_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing inc_dec_double_register {:#x}", Opcode);
    auto [high_byte, low_byte] = [&]() -> std::pair<Byte&, Byte&> {
        if constexpr(hi(Opcode) == 0x0) return {gb.processor.B, gb.processor.C};
        else if constexpr(hi(Opcode) == 0x1) return {gb.processor.D, gb.processor.E};
        else if constexpr(hi(Opcode) == 0x2) return {gb.processor.H, gb.processor.L};
    }();

    Double_Byte data = splice(high_byte, low_byte);

    constexpr Byte opcode_column = lo(Opcode);
    if constexpr(opcode_column == 0x3) data = data + 1;
    else if constexpr(opcode_column == 0xb) data = data - 1;
    high_byte = hi(data); 
    low_byte = lo(data);

    m_cycle_tick(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x3b>
void dec_sp(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing dec_sp {:#x}", Opcode);
    --gb.processor.stack_pointer;

    m_cycle_tick(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x33>
void inc_sp(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing inc_sp {:#x}", Opcode);
    ++gb.processor.stack_pointer;

    m_cycle_tick(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 
        0x04, 0x14, 0x24, 0x34, 0x0c, 0x1c, 0x2c, 0x3c,
        0x05, 0x15, 0x25, 0x35, 0x0d, 0x1d, 0x2d, 0x3d
    >
void inc_dec_single_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing inc_dec_single_register {:#x}", Opcode);
    decltype(auto) data = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x04 || Opcode == 0x05) return static_cast<Byte&>(gb.processor.B);
        else if constexpr(Opcode == 0x14 || Opcode == 0x15) return static_cast<Byte&>(gb.processor.D);
        else if constexpr(Opcode == 0x24 || Opcode == 0x25) return static_cast<Byte&>(gb.processor.H);
        else if constexpr(Opcode == 0x34 || Opcode == 0x35) { return cpu_memory_bus(gb, splice(gb.processor.H, gb.processor.L)); }
        else if constexpr(Opcode == 0x0c || Opcode == 0x0d) return static_cast<Byte&>(gb.processor.C);
        else if constexpr(Opcode == 0x1c || Opcode == 0x1d) return static_cast<Byte&>(gb.processor.E);
        else if constexpr(Opcode == 0x2c || Opcode == 0x2d) return static_cast<Byte&>(gb.processor.L);
        else if constexpr(Opcode == 0x3c || Opcode == 0x3d) return static_cast<Byte&>(gb.processor.A);
    }();

    constexpr Byte opcode_column = lo(Opcode);
    Byte register_value = data;
    if constexpr(opcode_column == 0x5 || opcode_column == 0xd) {
        gb.processor.half_carry_flag = half_carry_sub(register_value, static_cast<Byte>(1));
        register_value = register_value - 1;
        gb.processor.subtraction_flag = true;
    } 
    else if constexpr(opcode_column == 0x04 || opcode_column == 0xc) {
        gb.processor.half_carry_flag = half_carry_add(register_value, static_cast<Byte>(1));
        register_value = register_value + 1;
        gb.processor.subtraction_flag = false;
    }
    gb.processor.zero_flag = (register_value == 0);
    data = register_value;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x09, 0x19, 0x29, 0x39>
void arithmetic_hl_add(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing arithmetic_hl_add {:#x}", Opcode);
    Double_Byte data = [&]() {
        if constexpr(Opcode == 0x09) return splice(gb.processor.B, gb.processor.C);
        else if constexpr(Opcode == 0x19) return splice(gb.processor.D, gb.processor.E);
        else if constexpr(Opcode == 0x29) return splice(gb.processor.H, gb.processor.L);
        else if constexpr(Opcode == 0x39) return gb.processor.stack_pointer;
    }();

    Double_Byte HL = splice(gb.processor.H, gb.processor.L);
    Double_Byte sum = HL + data;

    gb.processor.subtraction_flag = false;
    gb.processor.half_carry_flag = ((HL & 0x0fff) + (data & 0x0fff) > 0x0fff);
    gb.processor.carry_flag = (static_cast<Double_Byte>(HL + data) < HL);

    gb.processor.H = hi(sum);
    gb.processor.L = lo(sum);

    m_cycle_tick(gb);
}

template<Byte Opcode>
    requires ( 
        (0x80 <= Opcode && Opcode <= 0xbf) ||
        is_one_of<Opcode, 0xc6, 0xd6, 0xe6, 0xf6, 0xce, 0xde, 0xee, 0xfe>
    )
void arithmetic_register(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing arithmetic_register {:#x}", Opcode);
    Byte data = [&]() {
        // Additional n8 arithmetic instructions
        if constexpr((Opcode & 0xf0) > 0xb0) return fetch(gb);

        // Standard register arithmetic instructions
        constexpr Byte RegCode = ((Opcode & 0x0f) % 0x08);
        if constexpr(RegCode == 0x00) return gb.processor.B;
        else if constexpr(RegCode == 0x01) return gb.processor.C;
        else if constexpr(RegCode == 0x02) return gb.processor.D;
        else if constexpr(RegCode == 0x03) return gb.processor.E;
        else if constexpr(RegCode == 0x04) return gb.processor.H;
        else if constexpr(RegCode == 0x05) return gb.processor.L;
        else if constexpr(RegCode == 0x06) return static_cast<Byte>(cpu_memory_bus(gb, splice(gb.processor.H, gb.processor.L)));
        else if constexpr(RegCode == 0x07) return gb.processor.A;
    }();

    constexpr Byte instruction_col = (Opcode & 0x0f) / 0x08;
    constexpr Byte instruction_row = (Opcode & 0xf0) % 0x40;
    if constexpr(instruction_row == 0x00 && instruction_col == 0x00) {
        gb.processor.half_carry_flag = half_carry_add(gb.processor.A, data);
        gb.processor.carry_flag = carry_add(gb.processor.A, data);

        gb.processor.A = gb.processor.A + data;

        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = false;
    }
    else if constexpr(instruction_row == 0x00 && instruction_col == 0x01) {
        bool new_carry_flag = carry_add(gb.processor.A, data, gb.processor.carry_flag);
        gb.processor.half_carry_flag = half_carry_add(gb.processor.A, data, gb.processor.carry_flag);

        gb.processor.A = gb.processor.A + data + gb.processor.carry_flag;

        gb.processor.carry_flag = new_carry_flag;
        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = false;
    }
    else if constexpr(instruction_row == 0x10 && instruction_col == 0x00) {
        gb.processor.carry_flag = carry_sub(gb.processor.A, data);
        gb.processor.half_carry_flag = half_carry_sub(gb.processor.A, data);

        gb.processor.A = gb.processor.A - data;

        gb.processor.zero_flag = (gb.processor.A == 0); 
        gb.processor.subtraction_flag = true;
    }
    else if constexpr(instruction_row == 0x10 && instruction_col == 0x01) {
        gb.processor.carry_flag = carry_sub(gb.processor.A, data, gb.processor.carry_flag);
        gb.processor.half_carry_flag = half_carry_sub(gb.processor.A, data, gb.processor.carry_flag);

        gb.processor.A = gb.processor.A - data - gb.processor.carry_flag;

        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = true;
    }
    else if constexpr(instruction_row == 0x20 && instruction_col == 0x00) {
        gb.processor.A = gb.processor.A & data;

        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = false;
        gb.processor.half_carry_flag = true;
        gb.processor.carry_flag = false;
    }
    else if constexpr(instruction_row == 0x20 && instruction_col == 0x01) {
        // For some reason no flags setting here?
        gb.processor.A = gb.processor.A ^ data;

        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = false;
        gb.processor.half_carry_flag = false;
        gb.processor.carry_flag = false;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x00) {
        // For some reason no flags setting here?
        gb.processor.A = gb.processor.A | data;

        gb.processor.zero_flag = (gb.processor.A == 0);
        gb.processor.subtraction_flag = false;
        gb.processor.half_carry_flag = false;
        gb.processor.carry_flag = false;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x01) {
        gb.processor.zero_flag = (gb.processor.A == data);
        gb.processor.subtraction_flag = true;
        gb.processor.half_carry_flag = half_carry_sub(gb.processor.A, data);
        gb.processor.carry_flag = carry_sub(gb.processor.A, data);
    }
}

template<Byte Opcode>
    requires (Opcode == 0xe8)
void add_sp_s8(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing add_sp_s8 {:#x}", Opcode);
    Signed_Byte operand = fetch(gb);

    gb.processor.zero_flag = false;
    gb.processor.subtraction_flag = false;
    // The carry flags are treated as if we are adding an unsigned byte to the lo byte of the stack pointer
    gb.processor.half_carry_flag = half_carry_add(lo(gb.processor.stack_pointer), static_cast<Byte>(operand));
    gb.processor.carry_flag = carry_add(lo(gb.processor.stack_pointer), static_cast<Byte>(operand));

    gb.processor.stack_pointer += operand;

    m_cycle_tick(gb, 2);
}

template<Byte Opcode>
    requires (Opcode == 0xf8)
void ld_hl_sp_s8(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ld_hl_sp_s8 {:#x}", Opcode);
    Signed_Byte operand = fetch(gb);

    gb.processor.zero_flag = false;
    gb.processor.subtraction_flag = false;
    // The carry flags are treated as if we are adding an unsigned byte to the lo byte of the stack pointer
    gb.processor.half_carry_flag = half_carry_add(lo(gb.processor.stack_pointer), static_cast<Byte>(operand));
    gb.processor.carry_flag = carry_add(lo(gb.processor.stack_pointer), static_cast<Byte>(operand));

    Double_Byte sum = gb.processor.stack_pointer + operand;
    m_cycle_tick(gb);

    gb.processor.H = hi(sum);
    gb.processor.L = lo(sum);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x07, 0x17>
void rotate_left(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing rotate_left {:#x}", Opcode);
    Byte most_significant_bit = (gb.processor.A >> 7);
    // Circular rotate
    if constexpr(Opcode == 0x07) gb.processor.A = (gb.processor.A << 1) | (most_significant_bit);
    // Rotate through carry flag
    else if constexpr(Opcode == 0x17) gb.processor.A = (gb.processor.A << 1) | (gb.processor.carry_flag);

    gb.processor.carry_flag = most_significant_bit;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x0f, 0x1f>
void rotate_right(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing rotate_right {:#x}", Opcode);
    Byte least_significant_bit = (gb.processor.A << 7);
    // Circular rotate
    if constexpr(Opcode == 0x07) gb.processor.A = (gb.processor.A >> 1) | (least_significant_bit);
    // Rotate through carry flag
    else if constexpr(Opcode == 0x1f) gb.processor.A = (gb.processor.A >> 1) | (gb.processor.carry_flag << 7);

    gb.processor.zero_flag = false;
    gb.processor.half_carry_flag = false;
    gb.processor.subtraction_flag = false;
    gb.processor.carry_flag = least_significant_bit;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc0, 0xd0, 0xc8, 0xd8, 0xc9>
void ret(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ret {:#x}", Opcode);
    bool flag = [&]() {
        if constexpr(Opcode != 0xc9) {
            m_cycle_tick(gb);
        }

        if constexpr(Opcode == 0xc0) return !gb.processor.zero_flag;
        else if constexpr(Opcode == 0xd0) return !gb.processor.carry_flag;
        else if constexpr(Opcode == 0xc8) return gb.processor.zero_flag;
        else if constexpr(Opcode == 0xd8) return gb.processor.carry_flag;
        else if constexpr(Opcode == 0xc9) return true;
    }();

    if(flag) {
        gb.processor.program_counter = pop(gb);
        m_cycle_tick(gb);
    }
}

template<Byte Opcode>
    requires(Opcode == 0xfb)
void ei(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ei {:#x}", Opcode);
    gb.processor.IME = true;
}

template<Byte Opcode>
    requires(Opcode == 0x27)
void daa(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing daa {:#x}", Opcode);
    // Last operation was an addition
    if(!gb.processor.subtraction_flag) {
        if(gb.processor.carry_flag || gb.processor.A > 0x99) {
            gb.processor.A += 0x60; 
            gb.processor.carry_flag = true;
        }

        if(gb.processor.half_carry_flag || (gb.processor.A & 0x0F) > 0x09) {
            gb.processor.A += 0x06;
        }
    }
    // Last operation was a subtraction
    else {
        if(gb.processor.carry_flag) {
            gb.processor.A -= 0x60;
        }
        if(gb.processor.half_carry_flag) {
            gb.processor.A -= 0x06;
        }
    }
    gb.processor.zero_flag = (gb.processor.A == 0);
    gb.processor.half_carry_flag = false;
}

template<Byte Opcode>
    requires(Opcode == 0x2f)
void cpl(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cpl {:#x}", Opcode);
    gb.processor.A = ~gb.processor.A;
    gb.processor.subtraction_flag = true;
    gb.processor.half_carry_flag = true;
}

template<Byte Opcode, typename Func>
inline constexpr void register_map_apply(GameBoy& gb, Func func) {
    constexpr Byte Regcode = Opcode % 8;
    if constexpr(Regcode == 0x0) func(gb.processor.B);
    else if constexpr(Regcode == 0x1) func(gb.processor.C);
    else if constexpr(Regcode == 0x2) func(gb.processor.D);
    else if constexpr(Regcode == 0x3) func(gb.processor.E);
    else if constexpr(Regcode == 0x4) func(gb.processor.H);
    else if constexpr(Regcode == 0x5) func(gb.processor.L);
    else if constexpr(Regcode == 0x6) func(cpu_memory_bus(gb, splice(gb.processor.H, gb.processor.L)));
    else if constexpr(Regcode == 0x7) func(gb.processor.A);
}

template<Byte Opcode>
    requires(0x00 <= Opcode && Opcode <= 0x07)
void cb_rotate_left_carry(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_rotate_left_carry {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        Byte most_significant_bit = (register_value & 0b10000000);
        gb.processor.carry_flag = most_significant_bit;

        memory = (register_value << 1) | most_significant_bit;
    });
}

template<Byte Opcode>   
    requires(0x08 <= Opcode && Opcode <= 0x0f)
void cb_rotate_right_carry(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_rotate_right_carry {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        Byte least_significant_bit = (register_value & 0b00000001);
        gb.processor.carry_flag = least_significant_bit;

        memory = (register_value >> 1) | least_significant_bit;
    });
}

template<Byte Opcode>
    requires(0x10 <= Opcode && Opcode <= 0x17)
void cb_rotate_left(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_rotate_left {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        memory = (memory << 1) | (gb.processor.carry_flag);
    });
}

template<Byte Opcode>
    requires(0x18 <= Opcode && Opcode <= 0x1f)
void cb_rotate_right(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_rotate_right {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        bool new_carry_flag = (register_value & 0b00000001);

        register_value = (register_value >> 1) | (gb.processor.carry_flag << 7);

        memory = register_value;

        gb.processor.carry_flag = new_carry_flag;
        gb.processor.zero_flag = (register_value == 0);
        gb.processor.half_carry_flag = false;
        gb.processor.subtraction_flag = false;
    });
}

template<Byte Opcode>
    requires(0x20 <= Opcode && Opcode <= 0x27)
void cb_shift_left_reset(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_shift_left_reset {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        Byte most_significant_bit = (register_value & 0b10000000);
        gb.processor.carry_flag = most_significant_bit;
        memory = (register_value << 1);
    });
}

template<Byte Opcode>
    requires(0x28 <= Opcode && Opcode <= 0x2f)
void cb_shift_right(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb_shift_right {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        Byte most_significant_bit = (register_value & 0b10000000);
        Byte least_significant_bit = (register_value & 0b00000001);
        gb.processor.carry_flag = least_significant_bit;
        memory = (register_value >> 1) | (most_significant_bit);
    });
}

template<Byte Opcode>
    requires (0x38 <= Opcode && Opcode <= 0x3f)
void cb_shift_right_reset(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb shift_right_reset {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        gb.processor.carry_flag = (register_value & 0b00000001);
        register_value = (register_value >> 1);
        memory = register_value;
        gb.processor.zero_flag = (register_value == 0);
        gb.processor.half_carry_flag = false;
        gb.processor.subtraction_flag = false;
    });
}

template<Byte Opcode>
    requires(0x30 <= Opcode && Opcode <= 0x37)
void swap(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing cb swap {:#x}", Opcode);
    register_map_apply<Opcode>(gb, [&](auto&& memory) {
        Byte register_value = memory;
        memory = (register_value << 4) | (register_value >> 4);
    });
}

// --- Dispatch table ---

using InstructionFunc = void (*)(GameBoy&);

const std::array<InstructionFunc, 256> prefixed_instruction_handler = {
/* 0x00 */ &cb_rotate_left_carry<0x00>, &cb_rotate_left_carry<0x01>, &cb_rotate_left_carry<0x02>, &cb_rotate_left_carry<0x03>,
/* 0x04 */ &cb_rotate_left_carry<0x04>, &cb_rotate_left_carry<0x05>, &cb_rotate_left_carry<0x06>, &cb_rotate_left_carry<0x07>,
/* 0x08 */ &cb_rotate_right_carry<0x08>, &cb_rotate_right_carry<0x09>, &cb_rotate_right_carry<0x0a>, &cb_rotate_right_carry<0x0b>,
/* 0x0c */ &cb_rotate_right_carry<0x0c>, &cb_rotate_right_carry<0x0d>, &cb_rotate_right_carry<0x0e>, &cb_rotate_right_carry<0x0f>,
/* 0x10 */ &cb_rotate_left<0x10>, &cb_rotate_left<0x11>, &cb_rotate_left<0x12>, &cb_rotate_left<0x13>,
/* 0x14 */ &cb_rotate_left<0x14>, &cb_rotate_left<0x15>, &cb_rotate_left<0x16>, &cb_rotate_left<0x17>,
/* 0x18 */ &cb_rotate_right<0x18>, &cb_rotate_right<0x19>, &cb_rotate_right<0x1a>, &cb_rotate_right<0x1b>,
/* 0x1c */ &cb_rotate_right<0x1c>, &cb_rotate_right<0x1d>, &cb_rotate_right<0x1e>, &cb_rotate_right<0x1f>,
/* 0x20 */ &cb_shift_left_reset<0x20>, &cb_shift_left_reset<0x21>, &cb_shift_left_reset<0x22>, &cb_shift_left_reset<0x23>,
/* 0x24 */ &cb_shift_left_reset<0x24>, &cb_shift_left_reset<0x25>, &cb_shift_left_reset<0x26>, &cb_shift_left_reset<0x27>,
/* 0x28 */ &cb_shift_right<0x28>, &cb_shift_right<0x29>, &cb_shift_right<0x2a>, &cb_shift_right<0x2b>,
/* 0x2c */ &cb_shift_right<0x2c>, &cb_shift_right<0x2d>, &cb_shift_right<0x2e>, &cb_shift_right<0x2f>,
/* 0x30 */ &swap<0x30>, &swap<0x31>, &swap<0x32>, &swap<0x33>,
/* 0x34 */ &swap<0x34>, &swap<0x35>, &swap<0x36>, &swap<0x37>,
/* 0x38 */ &cb_shift_right_reset<0x38>, &cb_shift_right_reset<0x39>, &cb_shift_right_reset<0x3a>, &cb_shift_right_reset<0x3b>,
/* 0x3c */ &cb_shift_right_reset<0x3c>, &cb_shift_right_reset<0x3d>, &cb_shift_right_reset<0x3e>, &cb_shift_right_reset<0x3f>,
/* 0x40 */ &cb_no_impl<0x40>, &cb_no_impl<0x41>, &cb_no_impl<0x42>, &cb_no_impl<0x43>,
/* 0x44 */ &cb_no_impl<0x44>, &cb_no_impl<0x45>, &cb_no_impl<0x46>, &cb_no_impl<0x47>,
/* 0x48 */ &cb_no_impl<0x48>, &cb_no_impl<0x49>, &cb_no_impl<0x4a>, &cb_no_impl<0x4b>,
/* 0x4c */ &cb_no_impl<0x4c>, &cb_no_impl<0x4d>, &cb_no_impl<0x4e>, &cb_no_impl<0x4f>,
/* 0x50 */ &cb_no_impl<0x50>, &cb_no_impl<0x51>, &cb_no_impl<0x52>, &cb_no_impl<0x53>,
/* 0x54 */ &cb_no_impl<0x54>, &cb_no_impl<0x55>, &cb_no_impl<0x56>, &cb_no_impl<0x57>,
/* 0x58 */ &cb_no_impl<0x58>, &cb_no_impl<0x59>, &cb_no_impl<0x5a>, &cb_no_impl<0x5b>,
/* 0x5c */ &cb_no_impl<0x5c>, &cb_no_impl<0x5d>, &cb_no_impl<0x5e>, &cb_no_impl<0x5f>,
/* 0x60 */ &cb_no_impl<0x60>, &cb_no_impl<0x61>, &cb_no_impl<0x62>, &cb_no_impl<0x63>,
/* 0x64 */ &cb_no_impl<0x64>, &cb_no_impl<0x65>, &cb_no_impl<0x66>, &cb_no_impl<0x67>,
/* 0x68 */ &cb_no_impl<0x68>, &cb_no_impl<0x69>, &cb_no_impl<0x6a>, &cb_no_impl<0x6b>,
/* 0x6c */ &cb_no_impl<0x6c>, &cb_no_impl<0x6d>, &cb_no_impl<0x6e>, &cb_no_impl<0x6f>,
/* 0x70 */ &cb_no_impl<0x70>, &cb_no_impl<0x71>, &cb_no_impl<0x72>, &cb_no_impl<0x73>,
/* 0x74 */ &cb_no_impl<0x74>, &cb_no_impl<0x75>, &cb_no_impl<0x76>, &cb_no_impl<0x77>,
/* 0x78 */ &cb_no_impl<0x78>, &cb_no_impl<0x79>, &cb_no_impl<0x7a>, &cb_no_impl<0x7b>,
/* 0x7c */ &cb_no_impl<0x7c>, &cb_no_impl<0x7d>, &cb_no_impl<0x7e>, &cb_no_impl<0x7f>,
/* 0x80 */ &cb_no_impl<0x80>, &cb_no_impl<0x81>, &cb_no_impl<0x82>, &cb_no_impl<0x83>,
/* 0x84 */ &cb_no_impl<0x84>, &cb_no_impl<0x85>, &cb_no_impl<0x86>, &cb_no_impl<0x87>,
/* 0x88 */ &cb_no_impl<0x88>, &cb_no_impl<0x89>, &cb_no_impl<0x8a>, &cb_no_impl<0x8b>,
/* 0x8c */ &cb_no_impl<0x8c>, &cb_no_impl<0x8d>, &cb_no_impl<0x8e>, &cb_no_impl<0x8f>,
/* 0x90 */ &cb_no_impl<0x90>, &cb_no_impl<0x91>, &cb_no_impl<0x92>, &cb_no_impl<0x93>,
/* 0x94 */ &cb_no_impl<0x94>, &cb_no_impl<0x95>, &cb_no_impl<0x96>, &cb_no_impl<0x97>,
/* 0x98 */ &cb_no_impl<0x98>, &cb_no_impl<0x99>, &cb_no_impl<0x9a>, &cb_no_impl<0x9b>,
/* 0x9c */ &cb_no_impl<0x9c>, &cb_no_impl<0x9d>, &cb_no_impl<0x9e>, &cb_no_impl<0x9f>,
/* 0xa0 */ &cb_no_impl<0xa0>, &cb_no_impl<0xa1>, &cb_no_impl<0xa2>, &cb_no_impl<0xa3>,
/* 0xa4 */ &cb_no_impl<0xa4>, &cb_no_impl<0xa5>, &cb_no_impl<0xa6>, &cb_no_impl<0xa7>,
/* 0xa8 */ &cb_no_impl<0xa8>, &cb_no_impl<0xa9>, &cb_no_impl<0xaa>, &cb_no_impl<0xab>,
/* 0xac */ &cb_no_impl<0xac>, &cb_no_impl<0xad>, &cb_no_impl<0xae>, &cb_no_impl<0xaf>,
/* 0xb0 */ &cb_no_impl<0xb0>, &cb_no_impl<0xb1>, &cb_no_impl<0xb2>, &cb_no_impl<0xb3>,
/* 0xb4 */ &cb_no_impl<0xb4>, &cb_no_impl<0xb5>, &cb_no_impl<0xb6>, &cb_no_impl<0xb7>,
/* 0xb8 */ &cb_no_impl<0xb8>, &cb_no_impl<0xb9>, &cb_no_impl<0xba>, &cb_no_impl<0xbb>,
/* 0xbc */ &cb_no_impl<0xbc>, &cb_no_impl<0xbd>, &cb_no_impl<0xbe>, &cb_no_impl<0xbf>,
/* 0xc0 */ &cb_no_impl<0xc0>, &cb_no_impl<0xc1>, &cb_no_impl<0xc2>, &cb_no_impl<0xc3>,
/* 0xc4 */ &cb_no_impl<0xc4>, &cb_no_impl<0xc5>, &cb_no_impl<0xc6>, &cb_no_impl<0xc7>,
/* 0xc8 */ &cb_no_impl<0xc8>, &cb_no_impl<0xc9>, &cb_no_impl<0xca>, &cb_no_impl<0xcb>,
/* 0xcc */ &cb_no_impl<0xcc>, &cb_no_impl<0xcd>, &cb_no_impl<0xce>, &cb_no_impl<0xcf>,
/* 0xd0 */ &cb_no_impl<0xd0>, &cb_no_impl<0xd1>, &cb_no_impl<0xd2>, &cb_no_impl<0xd3>,
/* 0xd4 */ &cb_no_impl<0xd4>, &cb_no_impl<0xd5>, &cb_no_impl<0xd6>, &cb_no_impl<0xd7>,
/* 0xd8 */ &cb_no_impl<0xd8>, &cb_no_impl<0xd9>, &cb_no_impl<0xda>, &cb_no_impl<0xdb>,
/* 0xdc */ &cb_no_impl<0xdc>, &cb_no_impl<0xdd>, &cb_no_impl<0xde>, &cb_no_impl<0xdf>,
/* 0xe0 */ &cb_no_impl<0xe0>, &cb_no_impl<0xe1>, &cb_no_impl<0xe2>, &cb_no_impl<0xe3>,
/* 0xe4 */ &cb_no_impl<0xe4>, &cb_no_impl<0xe5>, &cb_no_impl<0xe6>, &cb_no_impl<0xe7>,
/* 0xe8 */ &cb_no_impl<0xe8>, &cb_no_impl<0xe9>, &cb_no_impl<0xea>, &cb_no_impl<0xeb>,
/* 0xec */ &cb_no_impl<0xec>, &cb_no_impl<0xed>, &cb_no_impl<0xee>, &cb_no_impl<0xef>,
/* 0xf0 */ &cb_no_impl<0xf0>, &cb_no_impl<0xf1>, &cb_no_impl<0xf2>, &cb_no_impl<0xf3>,
/* 0xf4 */ &cb_no_impl<0xf4>, &cb_no_impl<0xf5>, &cb_no_impl<0xf6>, &cb_no_impl<0xf7>,
/* 0xf8 */ &cb_no_impl<0xf8>, &cb_no_impl<0xf9>, &cb_no_impl<0xfa>, &cb_no_impl<0xfb>,
/* 0xfc */ &cb_no_impl<0xfc>, &cb_no_impl<0xfd>, &cb_no_impl<0xfe>, &cb_no_impl<0xff>,
};

template<Byte Opcode>
    requires (Opcode == 0xcb)
void cb_prefix(GameBoy& gb) {
    Log::log<Log::Level::Verbose>("Executing ret {:#x}", Opcode);
    Byte next_instruction = fetch(gb);
    std::invoke(prefixed_instruction_handler[next_instruction], gb);
}

const std::array<InstructionFunc, 256> instruction_handler = {
/* 0x00 */ &nop,                           &ld_n16<0x01>,                  &ld_address_a<0x02>,            &inc_dec_double_register<0x03>,
/* 0x04 */ &inc_dec_single_register<0x04>, &inc_dec_single_register<0x05>, &ld_n8<0x06>,                   &rotate_left<0x07>,
/* 0x08 */ &ld_a16_sp<0x08>,                 &arithmetic_hl_add<0x09>,       &ld_address_a<0x0a>,            &inc_dec_double_register<0x0b>,
/* 0x0c */ &inc_dec_single_register<0x0c>, &inc_dec_single_register<0x0d>, &ld_n8<0x0e>,                   &rotate_right<0x0f>,
/* 0x10 */ &no_impl<0x10>,                 &ld_n16<0x11>,                  &ld_address_a<0x12>,            &inc_dec_double_register<0x13>,
/* 0x14 */ &inc_dec_single_register<0x14>, &inc_dec_single_register<0x15>, &ld_n8<0x16>,                   &rotate_left<0x17>,
/* 0x18 */ &jr<0x18>,                      &arithmetic_hl_add<0x19>,       &ld_address_a<0x1a>,            &inc_dec_double_register<0x1b>,
/* 0x1c */ &inc_dec_single_register<0x1c>, &inc_dec_single_register<0x1d>, &ld_n8<0x1e>,                   &rotate_right<0x1f>,
/* 0x20 */ &jr<0x20>,                      &ld_n16<0x21>,                  &ld_address_a<0x22>,            &inc_dec_double_register<0x23>,
/* 0x24 */ &inc_dec_single_register<0x24>, &inc_dec_single_register<0x25>, &ld_n8<0x26>,                   &daa<0x27>,
/* 0x28 */ &jr<0x28>,                      &arithmetic_hl_add<0x29>,       &ld_address_a<0x2a>,            &inc_dec_double_register<0x2b>,
/* 0x2c */ &inc_dec_single_register<0x2c>, &inc_dec_single_register<0x2d>, &ld_n8<0x2e>,                   &cpl<0x2f>,
/* 0x30 */ &jr<0x30>,                      &ld_n16<0x31>,                  &ld_address_a<0x32>,            &inc_sp<0x33>,
/* 0x34 */ &inc_dec_single_register<0x34>, &inc_dec_single_register<0x35>, &ld_n8<0x36>,                   &no_impl<0x37>,
/* 0x38 */ &jr<0x38>,                      &arithmetic_hl_add<0x39>,       &ld_address_a<0x3a>,            &dec_sp<0x3b>,
/* 0x3c */ &inc_dec_single_register<0x3c>, &inc_dec_single_register<0x3d>, &ld_n8<0x3e>,                   &no_impl<0x3f>,
/* 0x40 */ &ld_register<0x40>,             &ld_register<0x41>,             &ld_register<0x42>,             &ld_register<0x43>,
/* 0x44 */ &ld_register<0x44>,             &ld_register<0x45>,             &ld_register<0x46>,             &ld_register<0x47>,
/* 0x48 */ &ld_register<0x48>,             &ld_register<0x49>,             &ld_register<0x4a>,             &ld_register<0x4b>,
/* 0x4c */ &ld_register<0x4c>,             &ld_register<0x4d>,             &ld_register<0x4e>,             &ld_register<0x4f>,
/* 0x50 */ &ld_register<0x50>,             &ld_register<0x51>,             &ld_register<0x52>,             &ld_register<0x53>,
/* 0x54 */ &ld_register<0x54>,             &ld_register<0x55>,             &ld_register<0x56>,             &ld_register<0x57>,
/* 0x58 */ &ld_register<0x58>,             &ld_register<0x59>,             &ld_register<0x5a>,             &ld_register<0x5b>,
/* 0x5c */ &ld_register<0x5c>,             &ld_register<0x5d>,             &ld_register<0x5e>,             &ld_register<0x5f>,
/* 0x60 */ &ld_register<0x60>,             &ld_register<0x61>,             &ld_register<0x62>,             &ld_register<0x63>,
/* 0x64 */ &ld_register<0x64>,             &ld_register<0x65>,             &ld_register<0x66>,             &ld_register<0x67>,
/* 0x68 */ &ld_register<0x68>,             &ld_register<0x69>,             &ld_register<0x6a>,             &ld_register<0x6b>,
/* 0x6c */ &ld_register<0x6c>,             &ld_register<0x6d>,             &ld_register<0x6e>,             &ld_register<0x6f>,
/* 0x70 */ &ld_register<0x70>,             &ld_register<0x71>,             &ld_register<0x72>,             &ld_register<0x73>,
/* 0x74 */ &ld_register<0x74>,             &ld_register<0x75>,             &no_impl<0x76>,                 &ld_register<0x77>,
/* 0x78 */ &ld_register<0x78>,             &ld_register<0x79>,             &ld_register<0x7a>,             &ld_register<0x7b>,
/* 0x7c */ &ld_register<0x7c>,             &ld_register<0x7d>,             &ld_register<0x7e>,             &ld_register<0x7f>,
/* 0x80 */ &arithmetic_register<0x80>,     &arithmetic_register<0x81>,     &arithmetic_register<0x82>,     &arithmetic_register<0x83>,
/* 0x84 */ &arithmetic_register<0x84>,     &arithmetic_register<0x85>,     &arithmetic_register<0x86>,     &arithmetic_register<0x87>,
/* 0x88 */ &arithmetic_register<0x88>,     &arithmetic_register<0x89>,     &arithmetic_register<0x8a>,     &arithmetic_register<0x8b>,
/* 0x8c */ &arithmetic_register<0x8c>,     &arithmetic_register<0x8d>,     &arithmetic_register<0x8e>,     &arithmetic_register<0x8f>,
/* 0x90 */ &arithmetic_register<0x90>,     &arithmetic_register<0x91>,     &arithmetic_register<0x92>,     &arithmetic_register<0x93>,
/* 0x94 */ &arithmetic_register<0x94>,     &arithmetic_register<0x95>,     &arithmetic_register<0x96>,     &arithmetic_register<0x97>,
/* 0x98 */ &arithmetic_register<0x98>,     &arithmetic_register<0x99>,     &arithmetic_register<0x9a>,     &arithmetic_register<0x9b>,
/* 0x9c */ &arithmetic_register<0x9c>,     &arithmetic_register<0x9d>,     &arithmetic_register<0x9e>,     &arithmetic_register<0x9f>,
/* 0xa0 */ &arithmetic_register<0xa0>,     &arithmetic_register<0xa1>,     &arithmetic_register<0xa2>,     &arithmetic_register<0xa3>,
/* 0xa4 */ &arithmetic_register<0xa4>,     &arithmetic_register<0xa5>,     &arithmetic_register<0xa6>,     &arithmetic_register<0xa7>,
/* 0xa8 */ &arithmetic_register<0xa8>,     &arithmetic_register<0xa9>,     &arithmetic_register<0xaa>,     &arithmetic_register<0xab>,
/* 0xac */ &arithmetic_register<0xac>,     &arithmetic_register<0xad>,     &arithmetic_register<0xae>,     &arithmetic_register<0xaf>,
/* 0xb0 */ &arithmetic_register<0xb0>,     &arithmetic_register<0xb1>,     &arithmetic_register<0xb2>,     &arithmetic_register<0xb3>,
/* 0xb4 */ &arithmetic_register<0xb4>,     &arithmetic_register<0xb5>,     &arithmetic_register<0xb6>,     &arithmetic_register<0xb7>,
/* 0xb8 */ &arithmetic_register<0xb8>,     &arithmetic_register<0xb9>,     &arithmetic_register<0xba>,     &arithmetic_register<0xbb>,
/* 0xbc */ &arithmetic_register<0xbc>,     &arithmetic_register<0xbd>,     &arithmetic_register<0xbe>,     &arithmetic_register<0xbf>,
/* 0xc0 */ &ret<0xc0>,                     &pop_register<0xc1>,            &jp<0xc2>,                      &jp<0xc3>,
/* 0xc4 */ &call<0xc4>,                    &push_register<0xc5>,           &arithmetic_register<0xc6>,     &no_impl<0xc7>,
/* 0xc8 */ &ret<0xc8>,                     &ret<0xc9>,                     &jp<0xca>,                      &cb_prefix<0xcb>,
/* 0xcc */ &call<0xcc>,                    &call<0xcd>,                    &arithmetic_register<0xce>,     &no_impl<0xcf>,
/* 0xd0 */ &ret<0xd0>,                     &pop_register<0xd1>,            &jp<0xd2>,                      &no_impl<0xd3>,
/* 0xd4 */ &call<0xd4>,                    &push_register<0xd5>,           &arithmetic_register<0xd6>,     &no_impl<0xd7>,
/* 0xd8 */ &ret<0xd8>,                     &no_impl<0xd9>,                 &jp<0xda>,                      &no_impl<0xdb>,
/* 0xdc */ &call<0xdc>,                    &no_impl<0xdd>,                 &arithmetic_register<0xde>,     &no_impl<0xdf>,
/* 0xe0 */ &ld_address_a_misc<0xe0>,       &pop_register<0xe1>,            &ld_address_a_misc<0xe2>,       &no_impl<0xe3>,
/* 0xe4 */ &no_impl<0xe4>,                 &push_register<0xe5>,           &arithmetic_register<0xe6>,     &no_impl<0xe7>,
/* 0xe8 */ &add_sp_s8<0xe8>,                 &jp<0xe9>,                      &ld_address_a_misc<0xea>,       &no_impl<0xeb>,
/* 0xec */ &no_impl<0xec>,                 &no_impl<0xed>,                 &arithmetic_register<0xee>,     &no_impl<0xef>,
/* 0xf0 */ &ld_address_a_misc<0xf0>,       &pop_register<0xf1>,            &ld_address_a_misc<0xf2>,       &di,
/* 0xf4 */ &no_impl<0xf4>,                 &push_register<0xf5>,           &arithmetic_register<0xf6>,     &no_impl<0xf7>,
/* 0xf8 */ &ld_hl_sp_s8<0xf8>,                 &ld_sp_hl<0xf9>,                 &ld_address_a_misc<0xfa>,       &ei<0xfb>,
/* 0xfc */ &no_impl<0xfc>,                 &no_impl<0xfd>,                 &arithmetic_register<0xfe>,     &rst<0xff>,
};

} // anonymous namespace

void log_debug_state(GameBoy& gb) {
    Log::log<Log::Level::Doctor>(R"(A:{:02x} F:{:02x} B:{:02x} C:{:02x} D:{:02x} E:{:02x} H:{:02x} L:{:02x} SP:{:04x} PC:{:04x} PCMEM:{:02x},{:02x},{:02x},{:02x})",
        gb.processor.A, flags_as_byte(gb), gb.processor.B, gb.processor.C, gb.processor.D,
        gb.processor.E, gb.processor.H, gb.processor.L, gb.processor.stack_pointer, gb.processor.program_counter,
        static_cast<Byte>(memory_bus(gb, gb.processor.program_counter)), static_cast<Byte>(memory_bus(gb, gb.processor.program_counter + 1)),
        static_cast<Byte>(memory_bus(gb, gb.processor.program_counter + 2)), static_cast<Byte>(memory_bus(gb, gb.processor.program_counter + 3))
    );
}

void poll_and_handle_interrupts(GameBoy& gb) {

    // DMG interrupts: 
    // 0: VBlank, handler: 0x0040
    // 1: LCD, handler: 0x0048
    // 2: Timer, handler: 0x0050
    // 3: Serial, handler: 0x0058
    // 4: Joypad, handler: 0x0060
    Byte interrupt_enable = memory_bus(gb, 0xffff);
    Byte interrupt_flag = memory_bus(gb, 0xff0f);
    std::bitset<8> enable_list(interrupt_enable);
    std::bitset<8> request_list(interrupt_flag);
    std::array<Address, 5> interrupt_handlers = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060}; 

    for(int i = 0; i < 5; ++i) {
        if(gb.processor.IME) {
            if(enable_list[i] && request_list[i]) {
                Log::log<Log::Level::Verbose>("Interrupt Handler");

                gb.processor.IME = false;
                request_list[i] = 0x0;

                m_cycle_tick(gb, 2);

                memory_bus(gb, 0xff0f) = request_list.to_ulong();

                // Call instruction to the interrupt handler
                push(gb, gb.processor.program_counter);
                gb.processor.program_counter = interrupt_handlers[i];
            }
        }
    }
}

void SM83::fetch_decode_execute(GameBoy& gb) {
    while(true) {
        log_debug_state(gb);
        Log::log<Log::Level::Verbose>("Instruction Address {:#x}, ", gb.processor.program_counter);

        poll_and_handle_interrupts(gb);

        Byte next_instruction = fetch(gb);

        std::invoke(instruction_handler[next_instruction], gb);
        Log::log<Log::Level::Verbose>("End instruction loop\n");

        Log::log<Log::Level::Verbose>("{}", gb.processor.print_state());
        Log::log<Log::Level::Verbose>("{}", gb.timer.print_state());
        Log::log<Log::Level::Verbose>("{}", gb.interrupt.print_state());
    }
}
