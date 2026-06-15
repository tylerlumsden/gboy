#include <utility>
#include <array>
#include <stdexcept>
#include <format>
#include <functional>

#include "gameboy.hpp"
#include "memory.hpp"
#include "data_types.hpp"
#include "log.hpp"

using GB::GameBoy;

// --- Utilities ---

template <std::convertible_to<Byte>... Operands>
static bool half_carry_add(Byte a, Operands... operands) {
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
static bool carry_add(Byte a, Operands... operands) {
    std::array<Byte, sizeof...(operands)> operand_array{operands...};
    Byte running_sum = a;
    for(Byte operand : operand_array) {
        if(running_sum + operand < running_sum) {
            return true;
        }
        running_sum += operand;
    }
    return false;
}

template <std::convertible_to<Byte>... Operands>
static bool half_carry_sub(Byte a, Operands... operands) {
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
static bool carry_sub(Byte a, Operands... operands) {
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

static Byte flags_as_byte(GameBoy& gb) {
    Byte flags = 0x0;
    flags |= (gb.processor.carry_flag << 4);
    flags |= (gb.processor.half_carry_flag << 5);
    flags |= (gb.processor.subtraction_flag << 6);
    flags |= (gb.processor.zero_flag << 7);

    return flags;
}

static void byte_as_flags(GameBoy& gb, Byte data) {
    gb.processor.carry_flag = (data & 0b00010000);
    gb.processor.half_carry_flag = (data & 0b00100000);
    gb.processor.subtraction_flag = (data & 0b01000000);
    gb.processor.zero_flag = (data & 0b10000000);
}

static Byte fetch(GameBoy& gb) {
    Byte retval = read(gb, gb.processor.program_counter);
    ++gb.processor.program_counter;

    Log::log<Log::Level::Debug>("Fetched byte {:#x}", retval);    
    return retval;
}

static Double_Byte fetch_double(GameBoy& gb) {
    Byte low = fetch(gb);
    Byte high = fetch(gb);
    
    return splice(high, low);
}

static void push(GameBoy& gb, Double_Byte data) {
    Byte high = hi(data);
    Byte low = lo(data);


    --gb.processor.stack_pointer;
    memory_bus(gb, gb.processor.stack_pointer) = high;

    --gb.processor.stack_pointer;
    memory_bus(gb, gb.processor.stack_pointer) = low;
}

static Double_Byte pop(GameBoy& gb) {
    Byte low = memory_bus(gb, gb.processor.stack_pointer);
    ++gb.processor.stack_pointer;

    Byte high = memory_bus(gb, gb.processor.stack_pointer);
    ++gb.processor.stack_pointer;

    return splice(high, low);
}

// --- Instructions ---

template<Byte Opcode>
static void no_impl(GameBoy& gb) {
    throw std::logic_error(std::format("Opcode with byte value {:#x} is not implemented.\n", Opcode));
}

template<Byte Opcode>
static void cb_no_impl(GameBoy& gb) {
    throw std::logic_error(std::format("cb-prefixed Opcode with byte value {:#x} is not implemented.\n", Opcode));
}

static void nop(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing nop {:#x}", 0x00);
    // Should use 4 cycles
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xe9, 0xca, 0xda>
static void jp(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing jp {:#x}", 0xc3);

    auto data_call = [&]() {
        if constexpr(is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xca, 0xda>) return fetch_double(gb);
        else if constexpr(Opcode == 0xe9) return splice(gb.processor.H, gb.processor.L);
    };

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

    gb.processor.program_counter = data_call();
}

// TODO: implement variants
static void cp_n8(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing cp_n8 {:#x}", 0xfe);
    Byte num = fetch(gb);

    gb.processor.zero_flag = (num == gb.processor.A);
    gb.processor.subtraction_flag = true;
    gb.processor.half_carry_flag = lo(num) > lo(gb.processor.A);
    gb.processor.carry_flag = (num > gb.processor.A);
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0x20, 0x30, 0x18, 0x28, 0x38>
static void jr(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing jr {:#x}", Opcode);
    Signed_Byte relative_address = fetch(gb);
    if constexpr(Opcode == 0x20) {
        if(gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0x30) {
        if(gb.processor.carry_flag) return;
    }
    // Opcode 0x18 has no condition check, including constexpr for consistency
    else if constexpr(Opcode == 0x18);
    else if constexpr(Opcode == 0x28) {
        if(!gb.processor.zero_flag) return;
    }
    else if constexpr(Opcode == 0x30) {
        if(!gb.processor.carry_flag) return;
    }

    gb.processor.program_counter += relative_address;
}

// TODO: implement variants
template<Byte Opcode>
static void x_or(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing x_or {:#x}", Opcode);
    if constexpr(Opcode == 0xaf) {
        gb.processor.A = gb.processor.A ^ gb.processor.A;
    }
}

static void di(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing di {:#x}", 0xf3);
    gb.processor.IME = false;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x02, 0x0a, 0x12, 0x1a, 0x22, 0x2a, 0x32, 0x3a>
static void ld_address_a(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ld_address_a {:#x}", Opcode);
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
            Address addr_decrement = addr + 1;
            gb.processor.H = hi(addr_decrement);
            gb.processor.L = lo(addr_decrement);
            return addr;
        }
    }();

    constexpr Byte load_order = (Opcode & 0x0f);
    if constexpr(load_order == 0x02) {
        memory_bus(gb, address) = gb.processor.A;
    }
    else if constexpr(load_order == 0x0a) {
        gb.processor.A = memory_bus(gb, address);
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xe0, 0xe2, 0xea, 0xf0, 0xf2, 0xfa>
static void ld_address_a_misc(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ld_address_a_misc {:#x}", Opcode);
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
        memory_bus(gb, address) = gb.processor.A;
    }
    else if constexpr(load_order == 0xf0) {
        gb.processor.A = memory_bus(gb, address);
    }
}



template<Byte Opcode>
static void ld_register(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ld_register {:#x}", Opcode);
    decltype(auto) register_mapping = [&]<Byte Regcode>() -> decltype(auto) {
        if constexpr(Regcode == 0b000) return static_cast<Byte&>(gb.processor.B);
        else if constexpr(Regcode == 0b001) return static_cast<Byte&>(gb.processor.C);
        else if constexpr(Regcode == 0b010) return static_cast<Byte&>(gb.processor.D);
        else if constexpr(Regcode == 0b011) return static_cast<Byte&>(gb.processor.E);
        else if constexpr(Regcode == 0b100) return static_cast<Byte&>(gb.processor.H);
        else if constexpr(Regcode == 0b101) return static_cast<Byte&>(gb.processor.L);
        else if constexpr(Regcode == 0b110) return memory_bus(gb, splice(gb.processor.H, gb.processor.L));
        else if constexpr(Regcode == 0b111) return static_cast<Byte&>(gb.processor.A);
    };

    constexpr Byte DestCode = (Opcode & 0b00111000) >> 3;
    constexpr Byte SourceCode = (Opcode & 0b00000111);

    register_mapping.template operator()<DestCode>() = register_mapping.template operator()<SourceCode>();
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x06, 0x0e, 0x16, 0x1e, 0x26, 0x2e, 0x36, 0x3e>
static void ld_n8(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ld_n8 {:#x}", Opcode);
    decltype(auto) load_dest = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x06) return static_cast<Byte&>(gb.processor.B);
        if constexpr(Opcode == 0x0e) return static_cast<Byte&>(gb.processor.C);
        if constexpr(Opcode == 0x16) return static_cast<Byte&>(gb.processor.D);
        if constexpr(Opcode == 0x1e) return static_cast<Byte&>(gb.processor.E);
        if constexpr(Opcode == 0x26) return static_cast<Byte&>(gb.processor.H);
        if constexpr(Opcode == 0x2e) return static_cast<Byte&>(gb.processor.L);
        if constexpr(Opcode == 0x36) return memory_bus(gb, splice(gb.processor.H, gb.processor.L));
        if constexpr(Opcode == 0x3e) return static_cast<Byte&>(gb.processor.A);
    };

    load_dest() = fetch(gb);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x01, 0x11, 0x21, 0x31>
static void ld_n16(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ld_n16 {:#x}", Opcode);
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
static void swap(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing swap {:#x}", Opcode);
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x31) return gb.processor.A;
    }();

    Byte push_high = (data << 4);
    Byte push_low = (data >> 4);
    data = (push_high | push_low);
}

// TODO: implement variants
template<Byte Opcode>
static void rst(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing rst {:#x}", Opcode);
    Byte high_byte = hi(gb.processor.program_counter);
    Byte low_byte = lo(gb.processor.program_counter);
    write(gb, gb.processor.stack_pointer, high_byte);
    --gb.processor.stack_pointer;
    write(gb, gb.processor.stack_pointer, low_byte);
    --gb.processor.stack_pointer;

    if constexpr(Opcode == 0xff) {
        gb.processor.program_counter = 0x38;
    }
}

// TODO: implement variants
template<Byte Opcode>
static void inc(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing inc {:#x}", Opcode);
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x3c) return gb.processor.A;
    }();

    ++data;
}

// TODO: implement variants
template<Byte Opcode>
static void ldh(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ldh {:#x}", Opcode);
    if constexpr(Opcode == 0xe0) {
        Byte low_byte = fetch(gb);
        Byte high_byte = 0xff;

        Address addr = splice(high_byte, low_byte);

        write(gb, addr, gb.processor.A);
    }
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0xc4, 0xd4, 0xcc, 0xdc, 0xcd>
static void call(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing call {:#x}", Opcode);
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

    Byte low_byte = fetch(gb);
    Byte high_byte = fetch(gb);

    Address subroutine = splice(high_byte, low_byte);

    push(gb, gb.processor.program_counter);
    gb.processor.program_counter = subroutine;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc5, 0xd5, 0xe5, 0xf5>
static void push_register(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing push_register {:#x}", Opcode);
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
    Log::log<Log::Level::Debug>("Executing pop_register {:#x}", Opcode);
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
    Log::log<Log::Level::Debug>("Executing inc_dec_double_register {:#x}", Opcode);
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
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x3b>
void dec_sp(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing dec_sp {:#x}", Opcode);
    --gb.processor.stack_pointer;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x33>
void inc_sp(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing inc_sp {:#x}", Opcode);
    ++gb.processor.stack_pointer;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 
        0x04, 0x14, 0x24, 0x34, 0x0c, 0x1c, 0x2c, 0x3c,
        0x05, 0x15, 0x25, 0x35, 0x0d, 0x1d, 0x2d, 0x3d
    >
void inc_dec_single_register(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing inc_dec_single_register {:#x}", Opcode);
    decltype(auto) data = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x04 || Opcode == 0x05) return static_cast<Byte&>(gb.processor.B);
        else if constexpr(Opcode == 0x14 || Opcode == 0x15) return static_cast<Byte&>(gb.processor.D);
        else if constexpr(Opcode == 0x24 || Opcode == 0x25) return static_cast<Byte&>(gb.processor.H);
        else if constexpr(Opcode == 0x34 || Opcode == 0x35) return memory_bus(gb, splice(gb.processor.H, gb.processor.L));
        else if constexpr(Opcode == 0x0c || Opcode == 0x0d) return static_cast<Byte&>(gb.processor.C);
        else if constexpr(Opcode == 0x1c || Opcode == 0x1d) return static_cast<Byte&>(gb.processor.E);
        else if constexpr(Opcode == 0x2c || Opcode == 0x2d) return static_cast<Byte&>(gb.processor.L);
        else if constexpr(Opcode == 0x3c || Opcode == 0x3d) return static_cast<Byte&>(gb.processor.A);
    }();

    constexpr Byte opcode_column = lo(Opcode);
    if constexpr(opcode_column == 0x5 || opcode_column == 0xd) {
        gb.processor.half_carry_flag = half_carry_sub(data, static_cast<Byte>(1));
        data = data - 1;
        gb.processor.subtraction_flag = true;
    } 
    else if constexpr(opcode_column == 0x04 || opcode_column == 0xc) {
        gb.processor.half_carry_flag = half_carry_add(data, static_cast<Byte>(1));
        data = data + 1;
        gb.processor.subtraction_flag = false;
    }
    gb.processor.zero_flag = (data == 0);
}

template<Byte Opcode>
    requires ( 
        (0x80 <= Opcode && Opcode <= 0xbf) ||
        is_one_of<Opcode, 0xc6, 0xd6, 0xe6, 0xf6, 0xce, 0xde, 0xee, 0xfe>
    )
void arithmetic_register(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing arithmetic_register {:#x}", Opcode);
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
        else if constexpr(RegCode == 0x06) return static_cast<Byte>(memory_bus(gb, splice(gb.processor.H, gb.processor.L)));
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
        gb.processor.carry_flag = carry_add(gb.processor.A, data, gb.processor.carry_flag);
        gb.processor.half_carry_flag = half_carry_add(gb.processor.A, data, gb.processor.carry_flag);

        gb.processor.A = gb.processor.A + data + gb.processor.carry_flag;

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
    }
    else if constexpr(instruction_row == 0x20 && instruction_col == 0x01) {
        gb.processor.A = gb.processor.A ^ data;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x00) {
        gb.processor.A = gb.processor.A | data;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x01) {
        gb.processor.zero_flag = (gb.processor.A == data);
        gb.processor.subtraction_flag = true;
        gb.processor.half_carry_flag = half_carry_sub(gb.processor.A, data);
        gb.processor.carry_flag = carry_sub(gb.processor.A, data);
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x07, 0x17>
void rotate_left(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing rotate_left {:#x}", Opcode);
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
    Log::log<Log::Level::Debug>("Executing rotate_right {:#x}", Opcode);
    Byte least_significant_bit = (gb.processor.A << 7);
    // Circular rotate
    if constexpr(Opcode == 0x07) gb.processor.A = (gb.processor.A >> 1) | (least_significant_bit);
    // Rotate through carry flag
    else if constexpr(Opcode == 0x17) gb.processor.A = (gb.processor.A >> 1) | (gb.processor.carry_flag << 7);

    gb.processor.carry_flag = least_significant_bit;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc0, 0xd0, 0xc8, 0xd8, 0xc9>
void ret(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ret {:#x}", Opcode);
    bool flag = [&]() {
        if constexpr(Opcode == 0xc0) return !gb.processor.zero_flag;
        else if constexpr(Opcode == 0xd0) return !gb.processor.carry_flag;
        else if constexpr(Opcode == 0xc8) return gb.processor.zero_flag;
        else if constexpr(Opcode == 0xd8) return gb.processor.carry_flag;
        else if constexpr(Opcode == 0xc9) return true;
    }();

    if(flag) gb.processor.program_counter = pop(gb);
}

// --- Dispatch table ---

using InstructionFunc = void (*)(GameBoy&);

// Bind an explicit set of opcodes to a single instruction template.
// `make` maps a compile-time opcode to its handler, e.g. []<Byte Op>{ return &ret<Op>; }
template<Byte... Ops, typename Make>
constexpr void bind(std::array<InstructionFunc, 256>& handler, Make make) {
    ((handler[Ops] = make.template operator()<Ops>()), ...);
}

// Bind a contiguous inclusive range of opcodes [Lo, Hi] to a single instruction template.
template<Byte Lo, Byte Hi, typename Make>
constexpr void bind_range(std::array<InstructionFunc, 256>& handler, Make make) {
    [&]<std::size_t... Off>(std::index_sequence<Off...>) {
        ((handler[Lo + Off] = make.template operator()<Byte(Lo + Off)>()), ...);
    }(std::make_index_sequence<Hi - Lo + 1>{});
}

static const std::array<InstructionFunc, 256> prefixed_instruction_handler = [](){
    std::array<InstructionFunc, 256> handler =
    []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<InstructionFunc, 256>{ &cb_no_impl<I>... };
    }(std::make_index_sequence<256>{});

    return handler;
}();

template<Byte Opcode>
    requires (Opcode == 0xcb)
void cb_prefix(GameBoy& gb) {
    Log::log<Log::Level::Debug>("Executing ret {:#x}", Opcode);
    Byte next_instruction = fetch(gb);
    std::invoke(prefixed_instruction_handler[next_instruction], gb);
}

static const std::array<InstructionFunc, 256> instruction_handler = [](){
    std::array<InstructionFunc, 256> handler =
    []<std::size_t... I>(std::index_sequence<I...>) {
        return std::array<InstructionFunc, 256>{ &no_impl<I>... };
    }(std::make_index_sequence<256>{});

    handler[0x0] = &nop;
    handler[0xfe] = &cp_n8;
    handler[0x28] = &jr<0x28>;
    handler[0xaf] = &x_or<0xaf>;
    handler[0x18] = &jr<0x18>;
    handler[0xf3] = &di;
    handler[0xff] = &rst<0xff>;
    handler[0x3c] = &inc<0x3c>;
    handler[0xe0] = &ldh<0xe0>;

    //cb
    handler[0xcb] = &cb_prefix<0xcb>;

    // ret
    bind<0xc0, 0xd0, 0xc8, 0xd8, 0xc9>(handler, []<Byte Op>{ return &ret<Op>; });

    // jp
    bind<0xc2, 0xd2, 0xc3, 0xe9, 0xca, 0xda>(handler, []<Byte Op>{ return &jp<Op>; });

    // jr
    bind<0x20, 0x30, 0x18, 0x28, 0x38>(handler, []<Byte Op>{ return &jr<Op>; });

    // inc_double_register
    bind<0x03, 0x13, 0x23>(handler, []<Byte Op>{ return &inc_dec_double_register<Op>; });

    // inc_sp
    handler[0x33] = &inc_sp<0x33>;

    // dec_double_register
    bind<0x0b, 0x1b, 0x2b>(handler, []<Byte Op>{ return &inc_dec_double_register<Op>; });

    // dec_sp
    handler[0x3b] = &dec_sp<0x3b>;

    //inc_single_register
    bind<0x04, 0x14, 0x24, 0x34, 0x0c, 0x1c, 0x2c, 0x3c>(
        handler, []<Byte Op>{ return &inc_dec_single_register<Op>; });

    //dec_single_register
    bind<0x05, 0x15, 0x25, 0x35, 0x0d, 0x1d, 0x2d, 0x3d>(
        handler, []<Byte Op>{ return &inc_dec_single_register<Op>; });

    // call
    bind<0xc4, 0xd4, 0xcc, 0xdc, 0xcd>(handler, []<Byte Op>{ return &call<Op>; });

    // ld_address_a
    bind<0x02, 0x0a, 0x12, 0x1a, 0x22, 0x2a, 0x32, 0x3a>(
        handler, []<Byte Op>{ return &ld_address_a<Op>; });

    // ld_address_a_misc
    bind<0xe0, 0xe2, 0xea, 0xf0, 0xf2, 0xfa>(
        handler, []<Byte Op>{ return &ld_address_a_misc<Op>; });

    // ld_n8
    bind<0x06, 0x0e, 0x16, 0x1e, 0x26, 0x2e, 0x36, 0x3e>(
        handler, []<Byte Op>{ return &ld_n8<Op>; });

    // ld_n16
    bind<0x01, 0x11, 0x21, 0x31>(handler, []<Byte Op>{ return &ld_n16<Op>; });

    // pop_register
    bind<0xc1, 0xd1, 0xe1, 0xf1>(handler, []<Byte Op>{ return &pop_register<Op>; });

    // push_register
    bind<0xc5, 0xd5, 0xe5, 0xf5>(handler, []<Byte Op>{ return &push_register<Op>; });

    // arithmetic_register
    bind_range<0x80, 0xbf>(handler, []<Byte Op>{ return &arithmetic_register<Op>; });

    // Additional n8 arithmetic register
    bind<0xc6, 0xd6, 0xe6, 0xf6, 0xce, 0xde, 0xee, 0xfe>(
        handler, []<Byte Op>{ return &arithmetic_register<Op>; });

    // Rotate
    bind<0x07, 0x17>(handler, []<Byte Op>{ return &rotate_left<Op>; });
    bind<0x0f, 0x1f>(handler, []<Byte Op>{ return &rotate_right<Op>; });

    // ld_register
    bind_range<0x40, 0x75>(handler, []<Byte Op>{ return &ld_register<Op>; });
    // 0x76 is skipped -- it is a halt instruction
    bind_range<0x77, 0x7f>(handler, []<Byte Op>{ return &ld_register<Op>; });

    return handler;
}();

void SM83::fetch_decode_execute(GameBoy& gb) {
    while(true) {
        Log::log<Log::Level::Debug>("Instruction Address {:#x}, ", gb.processor.program_counter);
        Byte next_instruction = fetch(gb);

        std::invoke(instruction_handler[next_instruction], gb);
        Log::log<Log::Level::Debug>("End instruction loop\n");

        Log::log<Log::Level::Debug>("{}", gb.processor.print_state());
    }
}
