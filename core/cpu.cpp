#include "cpu.hpp"  
#include "data_types.hpp"
#include "log.hpp"

Byte SM83::CPU::fetch() {
    Byte retval = this->addressable_space.read(this->program_counter);
    ++program_counter;
    return retval;
}

void SM83::CPU::fetch_decode_execute() {
    while(true) {
        Log::log("Address {:#x}, ", this->program_counter);
        Byte next_instruction = this->fetch();
        Log::log("current instruction: {:#x}\n", next_instruction);

        std::invoke(this->instruction_handler[next_instruction], this);
    }
}

void SM83::CPU::nop() {
    // Should use 4 cycles
}

// TODO: implement variants
void SM83::CPU::jp() {
    Byte low_byte = this->fetch();
    Byte high_byte = this->fetch();

    Address jump_region = splice(high_byte, low_byte);
    this->program_counter = jump_region;
}

// TODO: implement variants
void SM83::CPU::cp() {
    Byte num = this->fetch();

    this->zero_flag = (num == this->A);
    this->subtraction_flag = true;
    this->half_carry_flag = lo(num) > lo(this->A);
    this->carry_flag = (num > this->A);
}

// TODO: implement variants
template <Byte Opcode>
void SM83::CPU::jr() {
    Signed_Byte relative_address = this->fetch();
    if constexpr(Opcode == 0x28) {
        if(!zero_flag) {
            return;
        }       
    }

    this->program_counter += relative_address;
}

// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::x_or() {
    if constexpr(Opcode == 0xaf) {
        this->A = A ^ A;
    }
}

void SM83::CPU::di() {
    this->IME = false;
}

// TODO: implement variants
template<Byte Opcode> 
void SM83::CPU::ld() {
    if constexpr(Opcode == 0x31) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->stack_pointer = splice(high_byte, low_byte);
    } else if constexpr(Opcode == 0xea) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        Address addr = splice(high_byte, low_byte);

        this->addressable_space.write(addr, this->A);
    }  else if constexpr(Opcode == 0x3e) {
        Byte data = this->fetch();
        this->A = data;
    }
}

// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::swap() {
    // The opcode determines which memory we swap on
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x31) return this->A;
    }();

    // Swap on the selected memory
    Byte push_high = (data << 4);
    Byte push_low = (data >> 4);
    data = (push_high | push_low);
}

// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::rst() {
    Byte high_byte = hi(this->program_counter);
    Byte low_byte = lo(this->program_counter);
    this->addressable_space.write(this->stack_pointer, high_byte);
    --this->stack_pointer;
    this->addressable_space.write(this->stack_pointer, low_byte);
    --this->stack_pointer;

    if constexpr(Opcode == 0xff) {
        this->program_counter = 0x38;
    }
}

// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::inc() {
    Byte& data = [&]() -> Byte& {
        if constexpr(Opcode == 0x3c) return this->A;
    }();

    ++data;   
}   

// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::ret() {
    // Pop an address from the stack
    ++this->stack_pointer;
    Byte low_byte = this->addressable_space.read(this->stack_pointer);
    ++this->stack_pointer;
    Byte high_byte = this->addressable_space.read(this->stack_pointer);

    // Set the program counter to the popped address
    this->program_counter = splice(high_byte, low_byte);
}


// TODO: implement variants
template<Byte Opcode>
void SM83::CPU::ldh() {
    if constexpr(Opcode == 0xe0) {
        Byte low_byte = this->fetch();
        Byte high_byte = 0xff;

        Address addr = splice(high_byte, low_byte);

        this->addressable_space.write(addr, this->A);
    }
}