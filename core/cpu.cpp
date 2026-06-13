#include <utility>
#include <array>
#include <stdexcept>
#include <format>

#include "cpu.hpp"
#include "data_types.hpp"
#include "log.hpp"

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

static Byte flags_as_byte(SM83::CPU& cpu) {
    Byte flags = 0x0;
    flags |= (cpu.carry_flag << 4);
    flags |= (cpu.half_carry_flag << 5);
    flags |= (cpu.subtraction_flag << 6);
    flags |= (cpu.zero_flag << 7);

    return flags;
}

static void byte_as_flags(SM83::CPU& cpu, Byte data) {
    cpu.carry_flag = (data & 0b00010000);
    cpu.half_carry_flag = (data & 0b00100000);
    cpu.subtraction_flag = (data & 0b01000000);
    cpu.zero_flag = (data & 0b10000000);
}

static Byte fetch(SM83::CPU& cpu) {
    Byte retval = cpu.addressable_space.read(cpu.program_counter);
    ++cpu.program_counter;

    Log::log<Log::Level::Debug>("Fetched byte {:#x}", retval);    
    return retval;
}

static Double_Byte fetch_double(SM83::CPU& cpu) {
    Byte low = fetch(cpu);
    Byte high = fetch(cpu);
    
    return splice(high, low);
}

static void push(SM83::CPU& cpu, Double_Byte data) {
    Byte high = hi(data);
    Byte low = lo(data);


    --cpu.stack_pointer;
    cpu.addressable_space[cpu.stack_pointer] = high;

    --cpu.stack_pointer;
    cpu.addressable_space[cpu.stack_pointer] = low;
}

static Double_Byte pop(SM83::CPU& cpu) {
    Byte low = cpu.addressable_space[cpu.stack_pointer];
    ++cpu.stack_pointer;

    Byte high = cpu.addressable_space[cpu.stack_pointer];
    ++cpu.stack_pointer;

    return splice(high, low);
}

// --- Instructions ---

template<Byte Opcode>
static void no_impl(SM83::CPU& cpu) {
    throw std::logic_error(std::format("Opcode with byte value {:#x} is not implemented.\n", Opcode));
}

static void nop(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing nop {:#x}", 0x00);
    // Should use 4 cycles
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xe9, 0xca, 0xda>
static void jp(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing jp {:#x}", 0xc3);

    auto data_call = [&]() {
        if constexpr(is_one_of<Opcode, 0xc2, 0xd2, 0xc3, 0xca, 0xda>) return fetch_double(cpu);
        else if constexpr(Opcode == 0xe9) return splice(cpu.H, cpu.L);
    };

    if constexpr(Opcode == 0xc2) {
        if(cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0xd2) {
        if(cpu.carry_flag) return;
    }
    else if constexpr(is_one_of<Opcode, 0xc3, 0xe9>) {
        // Conditionless jump
    }
    else if constexpr(Opcode == 0xca) {
        if(!cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0xda) {
        if(!cpu.carry_flag) return;
    }

    cpu.program_counter = data_call();
}

// TODO: implement variants
static void cp_n8(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing cp_n8 {:#x}", 0xfe);
    Byte num = fetch(cpu);

    cpu.zero_flag = (num == cpu.A);
    cpu.subtraction_flag = true;
    cpu.half_carry_flag = lo(num) > lo(cpu.A);
    cpu.carry_flag = (num > cpu.A);
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0x20, 0x30, 0x18, 0x28, 0x38>
static void jr(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing jr {:#x}", Opcode);
    Signed_Byte relative_address = fetch(cpu);
    if constexpr(Opcode == 0x20) {
        if(cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0x30) {
        if(cpu.carry_flag) return;
    }
    // Opcode 0x18 has no condition check, including constexpr for consistency
    else if constexpr(Opcode == 0x18);
    else if constexpr(Opcode == 0x28) {
        if(!cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0x30) {
        if(!cpu.carry_flag) return;
    }

    cpu.program_counter += relative_address;
}

// TODO: implement variants
template<Byte Opcode>
static void x_or(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing x_or {:#x}", Opcode);
    if constexpr(Opcode == 0xaf) {
        cpu.A = cpu.A ^ cpu.A;
    }
}

static void di(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing di {:#x}", 0xf3);
    cpu.IME = false;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x02, 0x0a, 0x12, 0x1a, 0x22, 0x2a, 0x32, 0x3a>
static void ld_address_a(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ld_address_a {:#x}", Opcode);
    constexpr Byte addr_type = (Opcode & 0xf0);
    Address address = [&]() {
        if constexpr(addr_type == 0x00) return splice(cpu.B, cpu.C);
        else if constexpr(addr_type == 0x10) return splice(cpu.D, cpu.E);
        else if constexpr(addr_type == 0x20) {
            Address addr = splice(cpu.H, cpu.L);
            Address addr_increment = addr + 1;
            cpu.H = hi(addr_increment);
            cpu.L = lo(addr_increment);
            return addr;
        }
        else if constexpr(addr_type == 0x30) {
            Address addr = splice(cpu.H, cpu.L);
            Address addr_decrement = addr + 1;
            cpu.H = hi(addr_decrement);
            cpu.L = lo(addr_decrement);
            return addr;
        }
    }();

    constexpr Byte load_order = (Opcode & 0x0f);
    if constexpr(load_order == 0x02) {
        cpu.addressable_space[address] = cpu.A;
    }
    else if constexpr(load_order == 0x0a) {
        cpu.A = cpu.addressable_space[address];
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xe0, 0xe2, 0xea, 0xf0, 0xf2, 0xfa>
static void ld_address_a_misc(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ld_address_a_misc {:#x}", Opcode);
    constexpr Byte addr_type = (Opcode & 0x0f);
    Address address = [&]() {
        if constexpr(addr_type == 0x00) {
            Byte addr = fetch(cpu);
            return 0xff00 | addr;
        }
        else if constexpr(addr_type == 0x02) {
            return 0xff00 | cpu.C;
        }
        else if constexpr(addr_type == 0x0a) {
            Byte low_byte = fetch(cpu);
            Byte high_byte = fetch(cpu);

            return splice(high_byte, low_byte);
        }
    }();

    constexpr Byte load_order = (Opcode & 0xf0);
    if constexpr(load_order == 0xe0) {
        cpu.addressable_space[address] = cpu.A;
    }
    else if constexpr(load_order == 0xf0) {
        cpu.A = cpu.addressable_space[address];
    }
}



template<Byte Opcode>
static void ld_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ld_register {:#x}", Opcode);
    decltype(auto) register_mapping = [&]<Byte Regcode>() -> decltype(auto) {
        if constexpr(Regcode == 0b000) return static_cast<Byte&>(cpu.B);
        else if constexpr(Regcode == 0b001) return static_cast<Byte&>(cpu.C);
        else if constexpr(Regcode == 0b010) return static_cast<Byte&>(cpu.D);
        else if constexpr(Regcode == 0b011) return static_cast<Byte&>(cpu.E);
        else if constexpr(Regcode == 0b100) return static_cast<Byte&>(cpu.H);
        else if constexpr(Regcode == 0b101) return static_cast<Byte&>(cpu.L);
        else if constexpr(Regcode == 0b110) return cpu.addressable_space[splice(cpu.H, cpu.L)];
        else if constexpr(Regcode == 0b111) return static_cast<Byte&>(cpu.A);
    };

    constexpr Byte DestCode = (Opcode & 0b00111000) >> 3;
    constexpr Byte SourceCode = (Opcode & 0b00000111);

    register_mapping.template operator()<DestCode>() = register_mapping.template operator()<SourceCode>();
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x06, 0x0e, 0x16, 0x1e, 0x26, 0x2e, 0x36, 0x3e>
static void ld_n8(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ld_n8 {:#x}", Opcode);
    decltype(auto) load_dest = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x06) return static_cast<Byte&>(cpu.B);
        if constexpr(Opcode == 0x0e) return static_cast<Byte&>(cpu.C);
        if constexpr(Opcode == 0x16) return static_cast<Byte&>(cpu.D);
        if constexpr(Opcode == 0x1e) return static_cast<Byte&>(cpu.E);
        if constexpr(Opcode == 0x26) return static_cast<Byte&>(cpu.H);
        if constexpr(Opcode == 0x2e) return static_cast<Byte&>(cpu.L);
        if constexpr(Opcode == 0x36) return cpu.addressable_space[splice(cpu.H, cpu.L)];
        if constexpr(Opcode == 0x3e) return static_cast<Byte&>(cpu.A);
    };

    load_dest() = fetch(cpu);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x01, 0x11, 0x21, 0x31>
static void ld_n16(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ld_n16 {:#x}", Opcode);
    Byte low = fetch(cpu);
    Byte high = fetch(cpu);

    if constexpr(Opcode == 0x31) {
        cpu.stack_pointer = splice(high, low);
    } else {
        auto [high_byte, low_byte] = [&]() -> std::pair<Byte&, Byte&> {
            if constexpr(Opcode == 0x01) return {cpu.B, cpu.C};
            else if constexpr(Opcode == 0x11) return {cpu.D, cpu.E};
            else if constexpr(Opcode == 0x21) return {cpu.H, cpu.L};
        }();

        high_byte = high; 
        low_byte = low;
    }
}

// TODO: implement variants
template<Byte Opcode>
static void swap(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing swap {:#x}", Opcode);
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x31) return cpu.A;
    }();

    Byte push_high = (data << 4);
    Byte push_low = (data >> 4);
    data = (push_high | push_low);
}

// TODO: implement variants
template<Byte Opcode>
static void rst(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing rst {:#x}", Opcode);
    Byte high_byte = hi(cpu.program_counter);
    Byte low_byte = lo(cpu.program_counter);
    cpu.addressable_space.write(cpu.stack_pointer, high_byte);
    --cpu.stack_pointer;
    cpu.addressable_space.write(cpu.stack_pointer, low_byte);
    --cpu.stack_pointer;

    if constexpr(Opcode == 0xff) {
        cpu.program_counter = 0x38;
    }
}

// TODO: implement variants
template<Byte Opcode>
static void inc(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing inc {:#x}", Opcode);
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x3c) return cpu.A;
    }();

    ++data;
}

// TODO: implement variants
template<Byte Opcode>
static void ret(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ret {:#x}", Opcode);

    cpu.program_counter = pop(cpu);
}

// TODO: implement variants
template<Byte Opcode>
static void ldh(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing ldh {:#x}", Opcode);
    if constexpr(Opcode == 0xe0) {
        Byte low_byte = fetch(cpu);
        Byte high_byte = 0xff;

        Address addr = splice(high_byte, low_byte);

        cpu.addressable_space.write(addr, cpu.A);
    }
}

// TODO: implement variants
template<Byte Opcode>
    requires is_one_of<Opcode, 0xc4, 0xd4, 0xcc, 0xdc, 0xcd>
static void call(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing call {:#x}", Opcode);
    if constexpr(Opcode == 0xc4) {
        if(cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0xd4) {
        if(cpu.carry_flag) return;
    }
    else if constexpr(Opcode == 0xcc) {
        if(!cpu.zero_flag) return;
    }
    else if constexpr(Opcode == 0xdc) {
        if(!cpu.carry_flag) return;
    }

    Byte low_byte = fetch(cpu);
    Byte high_byte = fetch(cpu);

    Address subroutine = splice(high_byte, low_byte);

    push(cpu, cpu.program_counter);
    cpu.program_counter = subroutine;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc5, 0xd5, 0xe5, 0xf5>
static void push_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing push_register {:#x}", Opcode);
    Double_Byte push_data = [&]() {
        if constexpr(Opcode == 0xc5) return splice(cpu.B, cpu.C);
        else if constexpr(Opcode == 0xd5) return splice(cpu.D, cpu.E);
        else if constexpr(Opcode == 0xe5) return splice(cpu.H, cpu.L);
        else if constexpr(Opcode == 0xf5) return splice(cpu.A, flags_as_byte(cpu));
    }();

    push(cpu, push_data);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc1, 0xd1, 0xe1, 0xf1>
void pop_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing pop_register {:#x}", Opcode);
    Double_Byte data = pop(cpu);

    if constexpr(Opcode == 0xc1) {
        cpu.B = hi(data);
        cpu.C = lo(data);
    }
    else if constexpr(Opcode == 0xd1) {
        cpu.D = hi(data);
        cpu.E = lo(data);
    }
    else if constexpr(Opcode == 0xe1) {
        cpu.H = hi(data);
        cpu.L = lo(data);
    }
    else if constexpr(Opcode == 0xf1) {
        cpu.A = hi(data);
        byte_as_flags(cpu, lo(data));
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x03, 0x13, 0x23, 0x0b, 0x1b, 0x2b>
void inc_dec_double_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing inc_dec_double_register {:#x}", Opcode);
    auto [high_byte, low_byte] = [&]() -> std::pair<Byte&, Byte&> {
        if constexpr(hi(Opcode) == 0x0) return {cpu.B, cpu.C};
        else if constexpr(hi(Opcode) == 0x1) return {cpu.D, cpu.E};
        else if constexpr(hi(Opcode) == 0x2) return {cpu.H, cpu.L};
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
void dec_sp(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing dec_sp {:#x}", Opcode);
    --cpu.stack_pointer;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x33>
void inc_sp(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing inc_sp {:#x}", Opcode);
    ++cpu.stack_pointer;
}

template<Byte Opcode>
    requires is_one_of<Opcode, 
        0x04, 0x14, 0x24, 0x34, 0x0c, 0x1c, 0x2c, 0x3c,
        0x05, 0x15, 0x25, 0x35, 0x0d, 0x1d, 0x2d, 0x3d
    >
void inc_dec_single_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing inc_dec_single_register {:#x}", Opcode);
    decltype(auto) data = [&]() -> decltype(auto) {
        if constexpr(Opcode == 0x04 || Opcode == 0x05) return static_cast<Byte&>(cpu.B);
        else if constexpr(Opcode == 0x14 || Opcode == 0x15) return static_cast<Byte&>(cpu.D);
        else if constexpr(Opcode == 0x24 || Opcode == 0x25) return static_cast<Byte&>(cpu.H);
        else if constexpr(Opcode == 0x34 || Opcode == 0x35) return cpu.addressable_space[splice(cpu.H, cpu.L)];
        else if constexpr(Opcode == 0x0c || Opcode == 0x0d) return static_cast<Byte&>(cpu.C);
        else if constexpr(Opcode == 0x1c || Opcode == 0x1d) return static_cast<Byte&>(cpu.E);
        else if constexpr(Opcode == 0x2c || Opcode == 0x2d) return static_cast<Byte&>(cpu.L);
        else if constexpr(Opcode == 0x3c || Opcode == 0x3d) return static_cast<Byte&>(cpu.A);
    }();

    constexpr Byte opcode_column = lo(Opcode);
    if constexpr(opcode_column == 0x5 || opcode_column == 0xd) {
        cpu.half_carry_flag = half_carry_sub(data, static_cast<Byte>(1));
        data = data - 1;
        cpu.subtraction_flag = true;
    } 
    else if constexpr(opcode_column == 0x04 || opcode_column == 0xc) {
        cpu.half_carry_flag = half_carry_add(data, static_cast<Byte>(1));
        data = data + 1;
        cpu.subtraction_flag = false;
    }
    cpu.zero_flag = (data == 0);
}

template<Byte Opcode>
    requires ( 
        (0x80 <= Opcode && Opcode <= 0xbf) ||
        is_one_of<Opcode, 0xc6, 0xd6, 0xe6, 0xf6, 0xce, 0xde, 0xee, 0xfe>
    )
void arithmetic_register(SM83::CPU& cpu) {
    Log::log<Log::Level::Debug>("Executing arithmetic_register {:#x}", Opcode);
    Byte data = [&]() {
        // Additional n8 arithmetic instructions
        if constexpr((Opcode & 0xf0) > 0xb0) return fetch(cpu);

        // Standard register arithmetic instructions
        constexpr Byte RegCode = ((Opcode & 0x0f) % 0x08);
        if constexpr(RegCode == 0x00) return cpu.B;
        else if constexpr(RegCode == 0x01) return cpu.C;
        else if constexpr(RegCode == 0x02) return cpu.D;
        else if constexpr(RegCode == 0x03) return cpu.E;
        else if constexpr(RegCode == 0x04) return cpu.H;
        else if constexpr(RegCode == 0x05) return cpu.L;
        else if constexpr(RegCode == 0x06) return static_cast<Byte>(cpu.addressable_space[splice(cpu.H, cpu.L)]);
        else if constexpr(RegCode == 0x07) return cpu.A;
    }();

    constexpr Byte instruction_col = (Opcode & 0x0f) / 0x08;
    constexpr Byte instruction_row = (Opcode & 0xf0) % 0x40;
    if constexpr(instruction_row == 0x00 && instruction_col == 0x00) {
        cpu.half_carry_flag = half_carry_add(cpu.A, data);
        cpu.carry_flag = carry_add(cpu.A, data);

        cpu.A = cpu.A + data;

        cpu.zero_flag = (cpu.A == 0);
        cpu.subtraction_flag = false;
    }
    else if constexpr(instruction_row == 0x00 && instruction_col == 0x01) {
        cpu.carry_flag = carry_add(cpu.A, data, cpu.carry_flag);
        cpu.half_carry_flag = half_carry_add(cpu.A, data, cpu.carry_flag);

        cpu.A = cpu.A + data + cpu.carry_flag;

        cpu.zero_flag = (cpu.A == 0);
        cpu.subtraction_flag = false;
    }
    else if constexpr(instruction_row == 0x10 && instruction_col == 0x00) {
        cpu.carry_flag = carry_sub(cpu.A, data);
        cpu.half_carry_flag = half_carry_sub(cpu.A, data);

        cpu.A = cpu.A - data;

        cpu.zero_flag = (cpu.A == 0); 
        cpu.subtraction_flag = true;
    }
    else if constexpr(instruction_row == 0x10 && instruction_col == 0x01) {
        cpu.carry_flag = carry_sub(cpu.A, data, cpu.carry_flag);
        cpu.half_carry_flag = half_carry_sub(cpu.A, data, cpu.carry_flag);

        cpu.A = cpu.A - data - cpu.carry_flag;

        cpu.zero_flag = (cpu.A == 0);
        cpu.subtraction_flag = true;
    }
    else if constexpr(instruction_row == 0x20 && instruction_col == 0x00) {
        cpu.A = cpu.A & data;
    }
    else if constexpr(instruction_row == 0x20 && instruction_col == 0x01) {
        cpu.A = cpu.A ^ data;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x00) {
        cpu.A = cpu.A | data;
    }
    else if constexpr(instruction_row == 0x30 && instruction_col == 0x01) {
        cpu.zero_flag = (cpu.A == data);
        cpu.subtraction_flag = true;
        cpu.half_carry_flag = half_carry_sub(cpu.A, data);
        cpu.carry_flag = carry_sub(cpu.A, data);
    }
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0x07, 0x17>
void rotate_left(SM83::CPU& cpu) {
    Byte most_significant_bit = (cpu.A >> 7);
    // Circular rotate
    if constexpr(Opcode == 0x07) cpu.A = (cpu.A << 8) | (most_significant_bit);
    // Rotate through carry flag
    else if constexpr(Opcode == 0x17) cpu.A = (cpu.A << 8) | (cpu.carry_flag);

    cpu.carry_flag = most_significant_bit;
}

// --- Dispatch table ---

using InstructionFunc = void (*)(SM83::CPU&);

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
    handler[0xc9] = &ret<0xc9>;
    handler[0xe0] = &ldh<0xe0>;

    // jp
    handler[0xc2] = &jp<0xc2>;
    handler[0xd2] = &jp<0xd2>;
    handler[0xc3] = &jp<0xc3>;
    handler[0xe9] = &jp<0xe9>;
    handler[0xca] = &jp<0xca>;
    handler[0xda] = &jp<0xda>;

    // jr
    handler[0x20] = &jr<0x20>;
    handler[0x30] = &jr<0x30>;
    handler[0x18] = &jr<0x18>;
    handler[0x28] = &jr<0x28>;
    handler[0x38] = &jr<0x38>;

    // inc_double_register
    handler[0x03] = &inc_dec_double_register<0x03>;
    handler[0x13] = &inc_dec_double_register<0x13>;
    handler[0x23] = &inc_dec_double_register<0x23>;

    // inc_sp
    handler[0x33] = &inc_sp<0x33>;

    // dec_double_register
    handler[0x0b] = &inc_dec_double_register<0x0b>;
    handler[0x1b] = &inc_dec_double_register<0x1b>;
    handler[0x2b] = &inc_dec_double_register<0x2b>;

    // dec_sp
    handler[0x3b] = &dec_sp<0x3b>;

    //inc_single_register
    handler[0x04] = &inc_dec_single_register<0x04>;
    handler[0x14] = &inc_dec_single_register<0x14>;
    handler[0x24] = &inc_dec_single_register<0x24>;
    handler[0x34] = &inc_dec_single_register<0x34>;
    handler[0x0c] = &inc_dec_single_register<0x0c>;
    handler[0x1c] = &inc_dec_single_register<0x1c>;
    handler[0x2c] = &inc_dec_single_register<0x2c>;
    handler[0x3c] = &inc_dec_single_register<0x3c>;

    //dec_single_register
    handler[0x05] = &inc_dec_single_register<0x05>;
    handler[0x15] = &inc_dec_single_register<0x15>;
    handler[0x25] = &inc_dec_single_register<0x25>;
    handler[0x35] = &inc_dec_single_register<0x35>;
    handler[0x0d] = &inc_dec_single_register<0x0d>;
    handler[0x1d] = &inc_dec_single_register<0x1d>;
    handler[0x2d] = &inc_dec_single_register<0x2d>;
    handler[0x3d] = &inc_dec_single_register<0x3d>;

    // call
    handler[0xc4] = &call<0xc4>;
    handler[0xd4] = &call<0xd4>;
    handler[0xcc] = &call<0xcc>;
    handler[0xdc] = &call<0xdc>;
    handler[0xcd] = &call<0xcd>;

    // ld_address_a
    handler[0x02] = &ld_address_a<0x02>;
    handler[0x0a] = &ld_address_a<0x0a>;
    handler[0x12] = &ld_address_a<0x12>;
    handler[0x1a] = &ld_address_a<0x1a>;
    handler[0x22] = &ld_address_a<0x22>;
    handler[0x2a] = &ld_address_a<0x2a>;
    handler[0x32] = &ld_address_a<0x32>;
    handler[0x3a] = &ld_address_a<0x3a>;

    // ld_address_a_misc
    handler[0xe0] = &ld_address_a_misc<0xe0>;
    handler[0xe2] = &ld_address_a_misc<0xe2>;
    handler[0xea] = &ld_address_a_misc<0xea>;
    handler[0xf0] = &ld_address_a_misc<0xf0>;
    handler[0xf2] = &ld_address_a_misc<0xf2>;
    handler[0xfa] = &ld_address_a_misc<0xfa>;

    // ld_n8
    handler[0x06] = &ld_n8<0x06>;
    handler[0x0e] = &ld_n8<0x0e>;
    handler[0x16] = &ld_n8<0x16>;
    handler[0x1e] = &ld_n8<0x1e>;
    handler[0x26] = &ld_n8<0x26>;
    handler[0x2e] = &ld_n8<0x2e>;
    handler[0x36] = &ld_n8<0x36>;
    handler[0x3e] = &ld_n8<0x3e>;


    // ld_n16
    handler[0x01] = &ld_n16<0x01>;
    handler[0x11] = &ld_n16<0x11>;
    handler[0x21] = &ld_n16<0x21>;
    handler[0x31] = &ld_n16<0x31>;
    
    // pop_register
    handler[0xc1] = &pop_register<0xc1>;
    handler[0xd1] = &pop_register<0xd1>;
    handler[0xe1] = &pop_register<0xe1>;
    handler[0xf1] = &pop_register<0xf1>;
    
    // push_register
    handler[0xc5] = &push_register<0xc5>;
    handler[0xd5] = &push_register<0xd5>;
    handler[0xe5] = &push_register<0xe5>;
    handler[0xf5] = &push_register<0xf5>;

    // arithmetic_register
    handler[0x80] = &arithmetic_register<0x80>;
    handler[0x81] = &arithmetic_register<0x81>;
    handler[0x82] = &arithmetic_register<0x82>;
    handler[0x83] = &arithmetic_register<0x83>;
    handler[0x84] = &arithmetic_register<0x84>;
    handler[0x85] = &arithmetic_register<0x85>;
    handler[0x86] = &arithmetic_register<0x86>;
    handler[0x87] = &arithmetic_register<0x87>;
    handler[0x88] = &arithmetic_register<0x88>;
    handler[0x89] = &arithmetic_register<0x89>;
    handler[0x8a] = &arithmetic_register<0x8a>;
    handler[0x8b] = &arithmetic_register<0x8b>;
    handler[0x8c] = &arithmetic_register<0x8c>;
    handler[0x8d] = &arithmetic_register<0x8d>;
    handler[0x8e] = &arithmetic_register<0x8e>;
    handler[0x8f] = &arithmetic_register<0x8f>;
    handler[0x90] = &arithmetic_register<0x90>;
    handler[0x91] = &arithmetic_register<0x91>;
    handler[0x92] = &arithmetic_register<0x92>;
    handler[0x93] = &arithmetic_register<0x93>;
    handler[0x94] = &arithmetic_register<0x94>;
    handler[0x95] = &arithmetic_register<0x95>;
    handler[0x96] = &arithmetic_register<0x96>;
    handler[0x97] = &arithmetic_register<0x97>;
    handler[0x98] = &arithmetic_register<0x98>;
    handler[0x99] = &arithmetic_register<0x99>;
    handler[0x9a] = &arithmetic_register<0x9a>;
    handler[0x9b] = &arithmetic_register<0x9b>;
    handler[0x9c] = &arithmetic_register<0x9c>;
    handler[0x9d] = &arithmetic_register<0x9d>;
    handler[0x9e] = &arithmetic_register<0x9e>;
    handler[0x9f] = &arithmetic_register<0x9f>;
    handler[0xa0] = &arithmetic_register<0xa0>;
    handler[0xa1] = &arithmetic_register<0xa1>;
    handler[0xa2] = &arithmetic_register<0xa2>;
    handler[0xa3] = &arithmetic_register<0xa3>;
    handler[0xa4] = &arithmetic_register<0xa4>;
    handler[0xa5] = &arithmetic_register<0xa5>;
    handler[0xa6] = &arithmetic_register<0xa6>;
    handler[0xa7] = &arithmetic_register<0xa7>;
    handler[0xa8] = &arithmetic_register<0xa8>;
    handler[0xa9] = &arithmetic_register<0xa9>;
    handler[0xaa] = &arithmetic_register<0xaa>;
    handler[0xab] = &arithmetic_register<0xab>;
    handler[0xac] = &arithmetic_register<0xac>;
    handler[0xad] = &arithmetic_register<0xad>;
    handler[0xae] = &arithmetic_register<0xae>;
    handler[0xaf] = &arithmetic_register<0xaf>;
    handler[0xb0] = &arithmetic_register<0xb0>;
    handler[0xb1] = &arithmetic_register<0xb1>;
    handler[0xb2] = &arithmetic_register<0xb2>;
    handler[0xb3] = &arithmetic_register<0xb3>;
    handler[0xb4] = &arithmetic_register<0xb4>;
    handler[0xb5] = &arithmetic_register<0xb5>;
    handler[0xb6] = &arithmetic_register<0xb6>;
    handler[0xb7] = &arithmetic_register<0xb7>;
    handler[0xb8] = &arithmetic_register<0xb8>;
    handler[0xb9] = &arithmetic_register<0xb9>;
    handler[0xba] = &arithmetic_register<0xba>;
    handler[0xbb] = &arithmetic_register<0xbb>;
    handler[0xbc] = &arithmetic_register<0xbc>;
    handler[0xbd] = &arithmetic_register<0xbd>;
    handler[0xbe] = &arithmetic_register<0xbe>;
    handler[0xbf] = &arithmetic_register<0xbf>;

    // Additional n8 arithmetic register
    handler[0xc6] = &arithmetic_register<0xc6>;
    handler[0xd6] = &arithmetic_register<0xd6>;
    handler[0xe6] = &arithmetic_register<0xe6>;
    handler[0xf6] = &arithmetic_register<0xf6>;
    handler[0xce] = &arithmetic_register<0xce>;
    handler[0xde] = &arithmetic_register<0xde>;
    handler[0xee] = &arithmetic_register<0xee>;
    handler[0xfe] = &arithmetic_register<0xfe>;

    // Rotate
    handler[0x07] = &rotate_left<0x07>;
    handler[0x17] = &rotate_left<0x17>;

    // ld_register
    handler[0x40] = &ld_register<0x40>;
    handler[0x41] = &ld_register<0x41>;
    handler[0x42] = &ld_register<0x42>;
    handler[0x43] = &ld_register<0x43>;
    handler[0x44] = &ld_register<0x44>;
    handler[0x45] = &ld_register<0x45>;
    handler[0x46] = &ld_register<0x46>;
    handler[0x47] = &ld_register<0x47>;
    handler[0x48] = &ld_register<0x48>;
    handler[0x49] = &ld_register<0x49>;
    handler[0x4a] = &ld_register<0x4a>;
    handler[0x4b] = &ld_register<0x4b>;
    handler[0x4c] = &ld_register<0x4c>;
    handler[0x4d] = &ld_register<0x4d>;
    handler[0x4e] = &ld_register<0x4e>;
    handler[0x4f] = &ld_register<0x4f>;
    handler[0x50] = &ld_register<0x50>;
    handler[0x51] = &ld_register<0x51>;
    handler[0x52] = &ld_register<0x52>;
    handler[0x53] = &ld_register<0x53>;
    handler[0x54] = &ld_register<0x54>;
    handler[0x55] = &ld_register<0x55>;
    handler[0x56] = &ld_register<0x56>;
    handler[0x57] = &ld_register<0x57>;
    handler[0x58] = &ld_register<0x58>;
    handler[0x59] = &ld_register<0x59>;
    handler[0x5a] = &ld_register<0x5a>;
    handler[0x5b] = &ld_register<0x5b>;
    handler[0x5c] = &ld_register<0x5c>;
    handler[0x5d] = &ld_register<0x5d>;
    handler[0x5e] = &ld_register<0x5e>;
    handler[0x5f] = &ld_register<0x5f>;
    handler[0x60] = &ld_register<0x60>;
    handler[0x61] = &ld_register<0x61>;
    handler[0x62] = &ld_register<0x62>;
    handler[0x63] = &ld_register<0x63>;
    handler[0x64] = &ld_register<0x64>;
    handler[0x65] = &ld_register<0x65>;
    handler[0x66] = &ld_register<0x66>;
    handler[0x67] = &ld_register<0x67>;
    handler[0x68] = &ld_register<0x68>;
    handler[0x69] = &ld_register<0x69>;
    handler[0x6a] = &ld_register<0x6a>;
    handler[0x6b] = &ld_register<0x6b>;
    handler[0x6c] = &ld_register<0x6c>;
    handler[0x6d] = &ld_register<0x6d>;
    handler[0x6e] = &ld_register<0x6e>;
    handler[0x6f] = &ld_register<0x6f>;
    handler[0x70] = &ld_register<0x70>;
    handler[0x71] = &ld_register<0x71>;
    handler[0x72] = &ld_register<0x72>;
    handler[0x73] = &ld_register<0x73>;
    handler[0x74] = &ld_register<0x74>;
    handler[0x75] = &ld_register<0x75>;
    // 0x76 is skipped -- it is a halt instruction
    handler[0x77] = &ld_register<0x77>;
    handler[0x78] = &ld_register<0x78>;
    handler[0x79] = &ld_register<0x79>;
    handler[0x7a] = &ld_register<0x7a>;
    handler[0x7b] = &ld_register<0x7b>;
    handler[0x7c] = &ld_register<0x7c>;
    handler[0x7d] = &ld_register<0x7d>;
    handler[0x7e] = &ld_register<0x7e>;
    handler[0x7f] = &ld_register<0x7f>;

    return handler;
}();

void SM83::CPU::fetch_decode_execute() {
    while(true) {
        Log::log<Log::Level::Debug>("Instruction Address {:#x}, ", this->program_counter);
        Byte next_instruction = fetch(*this);

        std::invoke(instruction_handler[next_instruction], *this);
        Log::log<Log::Level::Debug>("End instruction loop\n");

        Log::log<Log::Level::Debug>("{}", this->print_state());
    }
}
