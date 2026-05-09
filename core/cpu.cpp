#include "cpu.hpp"  
#include "data_types.hpp"
#include "log.hpp"

Byte SM83::CPU::fetch() {
    Byte retval = this->addressable_space[this->program_counter];
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

    Address jump_region = static_cast<Address>((high_byte << 8)) | low_byte;
    this->program_counter = jump_region;
}

// TODO: implement variants
void SM83::CPU::cp() {
    Byte num = this->fetch();

    this->zero_flag = (num == this->A);
    this->subtraction_flag = 1;
    this->half_carry_flag = (num & 0x0f) > (this->A & 0x0f);
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