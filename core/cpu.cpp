#include <utility>

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

template<Byte Opcode>
void SM83::CPU::ld_register() {
    auto register_mapping = [&]<Byte Regcode>() -> decltype(auto) {
        if constexpr(Regcode == 0b000) return static_cast<Byte&>(this->B);
        else if constexpr(Regcode == 0b001) return static_cast<Byte&>(this->C);
        else if constexpr(Regcode == 0b010) return static_cast<Byte&>(this->D);
        else if constexpr(Regcode == 0b011) return static_cast<Byte&>(this->E);
        else if constexpr(Regcode == 0b100) return static_cast<Byte&>(this->H);
        else if constexpr(Regcode == 0b101) return static_cast<Byte&>(this->L);
        else if constexpr(Regcode == 0b110) return this->addressable_space[splice(this->H, this->L)];
        else if constexpr(Regcode == 0b111) return static_cast<Byte&>(this->A);
    };

    constexpr Byte DestCode = (Opcode & 0b00111000) >> 3;
    constexpr Byte SourceCode = (Opcode & 0b00000111);

    register_mapping.template operator()<DestCode>() = register_mapping.template operator()<SourceCode>();
}

template<Byte Opcode> 
    requires is_one_of<Opcode, 0x01, 0x11, 0x21, 0x31>
void SM83::CPU::ld_n16() {
    Byte low = this->fetch();
    Byte high = this->fetch();

    if constexpr(Opcode == 0x31) {
        this->stack_pointer = splice(high, low);
    } else {
        auto [high_byte, low_byte] = [&]() -> std::pair<Byte&, Byte&> {
            if constexpr(Opcode == 0x01) return {this->B, this->C};
            else if constexpr(Opcode == 0x11) return {this->D, this->E};
            else if constexpr(Opcode == 0x21) return {this->H, this->L};
        }();

        high_byte = high; low_byte = low;
    }
}

// TODO: implement variants
template<Byte Opcode> 
void SM83::CPU::ld() {
    // Load an n16
    if constexpr(Opcode == 0x01) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->B = high_byte;
        this->C = low_byte;
    }
    else if constexpr(Opcode == 0x06) {
        Byte data = this->fetch();

        this->B = data;
    }
    else if constexpr(Opcode == 0x08) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->addressable_space.write(
            splice(high_byte, low_byte),
            this->stack_pointer
        );
    }
    else if constexpr(Opcode == 0x0a) {
        Byte data = this->addressable_space.read(
            splice(this->B, this->C)
        );

        this->A = data;
    }
    else if constexpr(Opcode == 0x0e) {
        Byte data = this->fetch();

        this->C = data;
    }
    else if constexpr(Opcode == 0x11) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->D = high_byte;
        this->E = low_byte;
    }
    else if constexpr(Opcode == 0x12) {
        Address addr = splice(this->D, this->E);
        
        this->addressable_space.write(addr, this->A);
    }
    else if constexpr(Opcode == 0x16) {
        Byte data = this->fetch();

        this->D = data;
    }
    else if constexpr(Opcode == 0x1a) {
        Byte data = this->addressable_space.read(
            splice(this->D, this->E)
        );

        this->A = data;
    }
    else if constexpr(Opcode == 0x1e) {
        Byte data = this->fetch();

        this->E = data;
    }
    else if constexpr(Opcode == 0x21) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->H = high_byte;
        this->L = low_byte;
    }
    else if constexpr(Opcode == 0x22) {
        Double_Byte HL = splice(this->H, this->L);
        this->addressable_space.write(HL, this->A);
        ++HL;

        this->H = hi(HL);
        this->L = lo(HL);
    }
    else if constexpr(Opcode == 0x26) {
        Byte data = fetch();

        this->H = data;
    }
    else if constexpr(Opcode == 0x2a) {
        Double_Byte HL = splice(this->H, this->L);
        this->A = addressable_space.read(HL);

        ++HL;
        this->H = hi(HL);
        this->L = lo(HL);
    }
    else if constexpr(Opcode == 0x2e) {
        Byte data = fetch();

        this->L = data;
    }
    else if constexpr(Opcode == 0x31) {
        Byte low_byte = this->fetch();
        Byte high_byte = this->fetch();

        this->stack_pointer = splice(high_byte, low_byte);
    } 
    else if constexpr(Opcode == 0x32) {
        Double_Byte HL = splice(this->H, this->L);
        this->addressable_space.write(HL, this->A);

        --HL;
        this->H = hi(HL);
        this->L = lo(HL);
    }
    else if constexpr(Opcode == 0x36) {
        Double_Byte HL = splice(this->H, this->L);
        Byte data = fetch();

        this->addressable_space.write(HL, data);
    }
    else if constexpr(Opcode == 0x3a) {
        Double_Byte HL = splice(this->H, this->L);
        this->A = this->addressable_space.read(HL);

        --HL;
        this->H = hi(HL);
        this->L = lo(HL);
    }
    else if constexpr(Opcode == 0x3e) {
        Byte data = fetch();

        this->A = data;
    }
    else if constexpr(Opcode == 0x40) {
        // no-op, assigns B to itself
    }
    else if constexpr(Opcode == 0x41) {
        this->B = this->C;
    }
    else if constexpr(Opcode == 0x42) {
        this->B = this->D;
    }
    else if constexpr(Opcode == 0x43) {
        this->B = this->E;
    }
    else if constexpr(Opcode == 0x44) {

    }

    else if constexpr(Opcode == 0xea) {
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
    this->pop_program_counter();
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

void SM83::CPU::push_program_counter() {
    Byte high = hi(this->program_counter);
    Byte low = lo(this->program_counter);

    this->addressable_space[this->stack_pointer] = high;
    --this->stack_pointer;

    this->addressable_space[this->stack_pointer] = low;
    --this->stack_pointer;
}

void SM83::CPU::pop_program_counter() {
    ++this->stack_pointer;
    Byte low = this->addressable_space[this->stack_pointer];

    ++this->stack_pointer;
    Byte high = this->addressable_space[this->stack_pointer];

    this->program_counter = splice(high, low);
}

template<Byte Opcode>
    requires is_one_of<Opcode, 0xc4, 0xd4, 0xcc, 0xdc, 0xcd>
void SM83::CPU::call() {
    if constexpr(Opcode == 0xc4) {
        if(this->zero_flag) return;
    } 
    else if constexpr(Opcode == 0xd4) {
        if(this->carry_flag) return;
    }
    else if constexpr(Opcode == 0xcc) {
        if(!this->zero_flag) return;
    }
    else if constexpr(Opcode == 0xdc) {
        if(!this->carry_flag) return;
    }

    Byte low_byte = this->fetch();
    Byte high_byte = this->fetch();

    Address subroutine = splice(high_byte, low_byte);

    this->push_program_counter();
    this->program_counter = subroutine;
}