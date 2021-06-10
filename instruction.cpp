#include "instruction.hpp"
#include <chrono>
#include <iostream>
#include <bit>
#include <concepts>


using namespace addressing;
using namespace utils;

/// BEGIN LOCAL FUNCTIONS FOR INTEGER/BCD CONVERSION
uint8_t conv_to_bcd(uint8_t n) {
    uint64_t bcd = n;
    for (int i = 0; i < 32; ++i) {
        for (int j = 0; j < 32; j += 4) {
            if (((bcd >> (32 + j)) & 0x0000000F) > 4) {
                bcd += ((uint64_t)3 << (32 + j));
            }
        }
        bcd <<= 1;
    }
    bcd >>= 32;
    return (uint8_t)bcd;
}

/// Converts a binary-coded decimal to an integer.
uint8_t conv_to_int(uint8_t bcd_data){
    return (((bcd_data & 0b11110000) >> 4) * 10) + (bcd_data & 0b00001111);
}

/// END LOCAL FUNCTIONS FOR INTEGER/BCD CONVERSION


/// ADC (Add with carry)
template<AddressingMode mode>
static cycles instructions::ADC(Cpu& cpu){
    constexpr cycles cyc = get_cycles<mode, 8>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},{2,3,4,4,4,4,6,5});
    auto data = load_addr<mode, NORMAL_MODE>(cpu);
    uint8_t number;
    uint16_t result;
    if (cpu.PS.D){ // BCD
        number = conv_to_int(data.first);
        result = (uint16_t)cpu.A + number + cpu.PS.C;
        if (result & 0x0080){
            cpu.A = conv_to_bcd((~result + 1) % 100); // 2's complement
        } else {
            cpu.A = conv_to_bcd(result % 100);
        }
    } else { // Normal
        number = data.first;
        result = (uint16_t)cpu.A + number + cpu.PS.C;
        cpu.A = result;
    }

    cpu.PS.C = result & 0x0100;
    if (!cpu.A) cpu.PS.Z = 1;
    if ((cpu.A & 0x80) ^ (cpu.PS.N << 7)) cpu.PS.V = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;

    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(mode) && data.second ? cyc + 1 : cyc;
}

/// AND (logical AND)
template<AddressingMode Mode>
static cycles instructions::AND(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y}, {2, 3, 4, 4, 4, 4, 6, 5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A &= data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

/// ASL (Arithmetic Shift Left)
template<AddressingMode Mode>
static cycles instructions::ASL(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X}, {2, 5, 6, 6, 7});
    std::pair<uint16_t, bool> data;
    uint16_t tmp;
    if (contains_modes<ACCUMULATOR>(Mode)){
        tmp = cpu.A << 1;
        cpu.A = tmp;
    } else {
        data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
        tmp = cpu.memory.get(data.first);
        cpu.memory.set(data.first, tmp << 1);
    }
    cpu.PS.C = tmp >> 8;
    if (!tmp) cpu.PS.Z = 1;
    if (tmp & 0x0080) cpu.PS.N = 1;
    return cyc;
}

/// BRANCH (Generic function for all relative branching in the 6502)
template<PSFlagType Flag, bool IsSet>
static cycles instructions::BRANCH(Cpu& cpu){
    uint16_t branch_location = cpu.PC + (int16_t)cpu.memory.get(cpu.PC++);
    if (is_flag<Flag, IsSet>(cpu)){
        if ((branch_location & 0xFF00) ^ (cpu.PC & 0xFF00)){ // Check if to a new page.
            cpu.PC = branch_location;
            return 4;
        }
        else {
            cpu.PC = branch_location;
            return 3;
        }
    }
    return 2;
}

/// BIT (Bit Test)
template<AddressingMode Mode>
static cycles instructions::BIT(Cpu &cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ABSOLUTE}, {3, 4});
    std::pair<uint16_t, bool> data = load_addr<Mode, NORMAL_MODE>(cpu);
    uint8_t value = cpu.A & data.first;
    if (value) cpu.PS.Z = 1;
    if (!value) cpu.PS.Z = 0;
    cpu.PS.N = value >> 7;
    cpu.PS.V = (value & 0x40) >> 6;
    return cyc;
}

/// BRK (Force Interrupt)
static cycles instructions::BRK(Cpu &cpu){
    constexpr cycles cyc = 7;
    cpu.PC++; // has an extra padding byte.
    cpu.PS.B = 0b11;
    uint8_t flags = cpu.PS.conv();
    cpu.push(cpu.PC >> 8);
    cpu.push(cpu.PC & 0x00FF);
    cpu.push(flags);
    cpu.PC = cpu.memory.get(0xFFFE) | (cpu.memory.get(0xFFFF) << 8);
    return cyc;
}

template<PSFlagType Flag, bool Value>
static cycles instructions::FLAGSET(Cpu& cpu){
    constexpr cycles cyc = 2;
    set_flag<Flag,Value>(cpu);
    return cyc;
}

template<AddressingMode Mode>
static cycles instructions::CMP(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    if (cpu.A >= data.first) cpu.PS.C = 1;
    if (cpu.A == data.first) cpu.PS.Z = 1;
    if (data.first & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX>
static cycles instructions::CMP_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ABSOLUTE}, {2,3,4});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    if (IsX){ // X Register
        if (cpu.X >= data.first) cpu.PS.C = 1;
        if (cpu.X == data.first) cpu.PS.Z = 1;
    } else { // Y Register
        if (cpu.Y >= data.first) cpu.PS.C = 1;
        if (cpu.Y == data.first) cpu.PS.Z = 1;
    }
    if (data.first & 0x80) cpu.PS.N = 1;
    return cyc;
}

template<AddressingMode Mode, bool IsIncrement>
static cycles instructions::INCDEC_MEMORY(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X}, {5,6,6,7});
    auto data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
    uint8_t result = IsIncrement ?  cpu.memory.get(data.first)+1 : cpu.memory.get(data.first)-1;
    if (!result) cpu.PS.Z = 1;
    if (result & 0x80) cpu.PS.N = 1;
    cpu.memory.set(data.first, result); // decrements
    return cyc;
}

template<bool IsX, bool IsIncrement>
static cycles instructions::INCDEC_REG(Cpu& cpu){
    constexpr cycles cyc = 2;
    uint8_t result;
    if (IsX)
        cpu.X = (result = IsIncrement ? cpu.X+1 : cpu.X-1);
     else
        cpu.Y = (result = IsIncrement ? cpu.Y+1 : cpu.Y-1);
    if (!result) cpu.PS.Z = 1;
    if (result & 0x80) cpu.PS.N = 1;
    return cyc;
}

/// EOR (Exclusive OR)
template<AddressingMode Mode>
static cycles instructions::EOR(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A ^= data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc+1 : cyc;
}

/// JMP (Jump)
template<AddressingMode Mode>
static cycles instructions::JMP(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ABSOLUTE, INDIRECT}, {3,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.PC = data.first;
    return cyc;
}

/// JSR (Jump to Subroutine)
template<AddressingMode Mode>
static cycles instructions::JSR(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ABSOLUTE}, {6}); // Done to ensure only ABSOLUTE is used in this instruction
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.push(cpu.PC >> 8);
    cpu.push((cpu.PC & 0x00FF) - 1);
    cpu.PC = data.first;
    return cyc;
}

template<AddressingMode Mode>
static cycles instructions::LDA(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A = data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX>
static cycles instructions::LOAD_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y}, {2,3,4,4,4,4});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    uint8_t result;
    if (IsX){ // X Register
        result = (cpu.X = data.first);
    } else { // Y Register
        result = (cpu.Y = data.first);
    }
    if (!result) cpu.PS.Z = 1;
    if (result & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, Bitshift::Enum ShiftType>
static cycles instructions::BITSHIFT(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X}, {2,5,6,6,7});
    if (contains_modes<ACCUMULATOR>(Mode)){
        switch (ShiftType){
            case Bitshift::ROTATE_LEFT:
                cpu.PS.C = cpu.A & 0x80;
                cpu.A = std::rotl(cpu.A, 1);
                break;
            case Bitshift::ROTATE_RIGHT:
                cpu.PS.C = cpu.A & 1;
                cpu.A = std::rotr(cpu.A, 1);
                break;
            case Bitshift::SHIFT_RIGHT:
                cpu.PS.C = cpu.A & 1;
                cpu.A >>= 1;
                break;
        }
        if (!cpu.A) cpu.PS.Z = 1;
        if (cpu.A & 0x80) cpu.PS.N = 1;
        return cyc;
    }
    auto data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
    cpu.PS.C = data.first & 1;
    uint8_t value = cpu.memory.get(data.first);
    uint8_t result;
    switch (ShiftType){
        case Bitshift::ROTATE_LEFT:
            result = std::rotl(value, 1);
            cpu.PS.C = value & 0x80;
            break;
        case Bitshift::ROTATE_RIGHT:
            result = std::rotr(value, 1);
            cpu.PS.C = value & 1;
            break;
        case Bitshift::SHIFT_RIGHT:
            result = value >> 1;
            cpu.PS.C = value & 1;
            break;

    }
    if (!result) cpu.PS.Z = 1;
    if (result & 0x80) cpu.PS.N = 1;
    return cyc;
}

/// NOP (No Operation)
static cycles instructions::NOP(Cpu& cpu){
    constexpr cycles cyc = 2; // IMPLIED
    return cyc;
}

/// ORA (Logical Inclusive OR)
template<AddressingMode Mode>
static cycles instructions::ORA(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A |= data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<bool IsAcc>
static cycles instructions::PUSH_REG(Cpu& cpu){
    constexpr cycles cyc = 3; // IMPLIED
    if (IsAcc)
        cpu.push(cpu.A);
    else
        cpu.push(cpu.PS.conv());
    return cyc;
}


template<bool IsAcc>
static cycles instructions::PULL_REG(Cpu& cpu){
    constexpr cycles cyc = 4; // IMPLIED
    uint8_t result = cpu.pop();
    if (IsAcc){
        cpu.A = result;
        if (!cpu.A) cpu.PS.Z = 1;
        if (cpu.A & 0x80) cpu.PS.N = 1;
    } else {
        cpu.PS.set(result);
    }
    return cyc;
}

/// RTI (Return from Interrupt)
static cycles instructions::RTI(Cpu& cpu){
    constexpr cycles cyc = 6; // IMPLIED
    cpu.PS.set(cpu.pop());
    cpu.PC = cpu.pop() | (cpu.pop() << 8);
    return cyc;
}

/// RTS (Return from Subroutine)
static cycles instructions::RTS(Cpu& cpu){
    constexpr cycles cyc = 6; // IMPLIED
    cpu.PC = cpu.pop() | (cpu.pop() << 8);
    cpu.PC += 1;
    return cyc;
}

/// SBC (Subtract with Carry)
template<AddressingMode Mode>
static cycles instructions::SBC(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    uint8_t number;
    uint8_t result;
    if (cpu.PS.D){ // BCD
        number = ~conv_to_int(data.first) + 1; // 2's complement
        result = cpu.A + number - !cpu.PS.C;
        if (result & 0x0080) {
            cpu.A = conv_to_bcd((~result + 1) % 100);
            cpu.PS.C = 0;
        } else {
            cpu.A = conv_to_bcd(result % 100);
        }
    } else { // Normal
        number = ~data.first + 1; // 2's complement
        result = cpu.A + number - !cpu.PS.C;
        cpu.A = result;
    }
    if (!cpu.A)
        cpu.PS.Z = 1;
    else
        cpu.PS.Z = 0;

    if ((cpu.A & 0x80) ^ (cpu.PS.N << 7)) cpu.PS.V = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}


template<AddressingMode Mode, Register::Enum Reg>
static cycles instructions::STORE_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y}, {3,4,4,5,5,6,6});
    auto data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
    switch (Reg){
        case Register::A:
            cpu.memory.set(data.first, cpu.A);
            break;
        case Register::X:
            cpu.memory.set(data.first, cpu.X);
            break;
        case Register::Y:
            cpu.memory.set(data.first, cpu.Y);
            break;
        default:
            throw std::runtime_error("Invalid Register type in STORE_REG template parameter `Reg`.");
    }
    return cyc;
}

template<Register::Enum FromReg, Register::Enum ToReg>
static cycles instructions::TRANSFER_REG(Cpu& cpu){
    static const char error_words[] = "Invalid Register type in TRANSFER_REF template parameter `ToReg`.";
    constexpr cycles cyc = 2;
    switch (FromReg) {
        case Register::A:
            switch (ToReg){
                case Register::X:
                    cpu.X = cpu.A;
                    if (!cpu.X) cpu.PS.Z = 1;
                    if (cpu.X & 0x80) cpu.PS.N = 1;
                    break;
                case Register::Y:
                    cpu.Y = cpu.A;
                    if (!cpu.Y) cpu.PS.Z = 1;
                    if (cpu.Y & 0x80) cpu.PS.N = 1;
                    break;
                default:
                    throw std::runtime_error(error_words);
            }
            break;
        case Register::X:
            switch (ToReg){
                case Register::A:
                    cpu.A = cpu.X;
                    if (!cpu.A) cpu.PS.Z = 1;
                    if (cpu.A & 0x80) cpu.PS.N = 1;
                    break;
                case Register::SP:
                    cpu.SP = cpu.X;
                    break;
                default:
                    throw std::runtime_error(error_words);
            }
            break;
        case Register::Y:
            if (ToReg == Register::A)
                cpu.A = cpu.Y;
            else
                throw std::runtime_error(error_words);
            if (!cpu.A) cpu.PS.Z = 1;
            if (cpu.A & 0x80) cpu.PS.N = 1;
            break;
        case Register::SP:
            if (ToReg == Register::X)
                cpu.X = cpu.SP;
            else
                throw std::runtime_error(error_words);
            if (!cpu.X) cpu.PS.Z = 1;
            if (cpu.X & 0x80) cpu.PS.N = 1;
            break;
    }
    return cyc;
}

InstructionTable::InstructionTable(){
    using namespace instructions;
    auto time_begin = std::chrono::steady_clock::now();

    /// ADC
    create_instructions(
            {0x69, 0x65, 0x75, 0x6D, 0x7D, 0x79, 0x61, 0x71},
            {ADC<IMMEDIATE>, ADC<ZERO_PAGE>, ADC<ZERO_PAGE_XY>, ADC<ABSOLUTE>, ADC<ABSOLUTE_X>, ADC<ABSOLUTE_Y>, ADC<INDIRECT_X>, ADC<INDIRECT_Y>},
            "ADC");
    /// AND
    create_instructions({0x29, 0x25, 0x35, 0x2D, 0x3D, 0x39, 0x21, 0x31},
                           {AND<IMMEDIATE>, AND<ZERO_PAGE>, AND<ZERO_PAGE_XY>, AND<ABSOLUTE>, AND<ABSOLUTE_X>, AND<ABSOLUTE_Y>, AND<INDIRECT_X>, AND<INDIRECT_Y>},
                           "AND");
    /// ASL
    create_instructions({0x0A, 0x06, 0x16, 0x0E, 0x1E},
                        {ASL<ACCUMULATOR>, ASL<ZERO_PAGE>, ASL<ZERO_PAGE_XY>, ASL<ABSOLUTE>, ASL<ABSOLUTE_X>},
                        "ASL");

    /// BCC
    create_instructions({0x90}, {BRANCH<CARRY_FLAG, false>}, "BCC");

    /// BCS
    create_instructions({0xB0}, {BRANCH<CARRY_FLAG, true>}, "BCS");

    /// BEQ
    create_instructions({0xF0}, {BRANCH<ZERO_FLAG, true>}, "BEQ");

    /// BIT
    create_instructions({0x24, 0x2C}, {BIT<ZERO_PAGE>, BIT<ABSOLUTE>}, "BIT");

    /// BMI
    create_instructions({0x30}, {BRANCH<NEGATIVE_FLAG, true>}, "BMI");

    /// BNE
    create_instructions({0xD0}, {BRANCH<ZERO_FLAG, false>}, "BNE");

    /// BPL
    create_instructions({0x10}, {BRANCH<NEGATIVE_FLAG, false>}, "BPL");

    /// BRK
    create_instructions({0x00}, {BRK}, "BRK");

    /// BVC (Branch if Overflow Clear)
    create_instructions({0x50}, {BRANCH<OVERFLOW_FLAG, false>}, "BVC");

    /// BVS (Branch if Overflow Set)
    create_instructions({0x70}, {BRANCH<OVERFLOW_FLAG, true>}, "BVS");

    /// CLC (Clear Carry Flag)
    create_instructions({0x18}, {FLAGSET<CARRY_FLAG, false>}, "CLC");

    /// CLD (Clear Decimal Mode)
    create_instructions({0xD8}, {FLAGSET<DECIMAL_FLAG, false>}, "CLD");

    /// CLI (Clear Interrupt Disable)
    create_instructions({0x58}, {FLAGSET<INTERRUPT_DISABLE_FLAG, false>}, "CLI");

    /// CLV (Clear Overflow Flag)
    create_instructions({0xB8}, {FLAGSET<OVERFLOW_FLAG, false>}, "CLV");

    /// CMP (Compare Accumulator)
    create_instructions({0xC9, 0xC5, 0xD5, 0xCD, 0xDD, 0xD9, 0xC1, 0xD1},
                        {CMP<IMMEDIATE>, CMP<ZERO_PAGE>, CMP<ZERO_PAGE_XY>, CMP<ABSOLUTE>, CMP<ABSOLUTE_X>, CMP<ABSOLUTE_Y>, CMP<INDIRECT_X>, CMP<INDIRECT_Y>},
                        "CMP");

    /// CPX (Compare X Register)
    create_instructions({0xE0, 0xE4, 0xEC}, {CMP_REG<IMMEDIATE, true>, CMP_REG<ZERO_PAGE, true>, CMP_REG<ABSOLUTE, true>}, "CPX");

    /// CPY (Compare Y Register
    create_instructions({0xC0, 0xC4, 0xCC}, {CMP_REG<IMMEDIATE, false>, CMP_REG<ZERO_PAGE, false>, CMP_REG<ABSOLUTE, false>}, "CPY");

    /// DEC (Decrement Memory)
    create_instructions( {0xC6, 0xD6, 0xCE, 0xDE},
                         {INCDEC_MEMORY<ZERO_PAGE, false>, INCDEC_MEMORY<ZERO_PAGE_XY, false>, INCDEC_MEMORY<ABSOLUTE, false>, INCDEC_MEMORY<ABSOLUTE_X, false>},
                         "DEC");

    /// DEX (Decrement X Register)
    create_instructions({0xCA}, {INCDEC_REG<true, false>}, "DEX");

    /// DEY (Decrement Y Register)
    create_instructions({0x88}, {INCDEC_REG<false, false>}, "DEY");

    /// EOR (Exclusive OR)
    create_instructions({0x49, 0x45, 0x55, 0x4D, 0x5D, 0x59, 0x41, 0x51},
                        {EOR<IMMEDIATE>, EOR<ZERO_PAGE>, EOR<ZERO_PAGE_XY>, EOR<ABSOLUTE>, EOR<ABSOLUTE_X>, EOR<ABSOLUTE_Y>, EOR<INDIRECT_X>, EOR<INDIRECT_Y>},
                        "EOR");

    /// INC (Increment Memory)
    create_instructions({0xE6, 0xF6, 0xEE, 0xFE}, {INCDEC_MEMORY<ZERO_PAGE, true>, INCDEC_MEMORY<ZERO_PAGE_XY, true>, INCDEC_MEMORY<ABSOLUTE, true>, INCDEC_MEMORY<ABSOLUTE_X, true>},
                        "INC");

    /// INX (Increment X Register)
    create_instructions({0xE8}, {INCDEC_REG<true, true>}, "INX");

    /// INY (Increment Y Register)
    create_instructions({0xC8}, {INCDEC_REG<false, true>}, "INY");

    /// JMP (Jump)
    create_instructions({0x4C, 0x6C}, {JMP<ABSOLUTE>, JMP<INDIRECT>}, "JMP");

    /// JSR (Jump to Subroutine)
    create_instructions({0x20}, {JSR<ABSOLUTE>}, "JSR");

    /// LDA (Load Accumulator)
    create_instructions({0xA9, 0xA5, 0xB5, 0xAD, 0xBD, 0xB9, 0xA1, 0xB1},
                        {LDA<IMMEDIATE>, LDA<ZERO_PAGE>, LDA<ZERO_PAGE_XY>, LDA<ABSOLUTE>, LDA<ABSOLUTE_X>, LDA<ABSOLUTE_Y>, LDA<INDIRECT_X>, LDA<INDIRECT_Y>},
                        "LDA");

    /// LDX (Load X Register)
    create_instructions({0xA2, 0xA6, 0xB6, 0xAE, 0xBE},
                        {LOAD_REG<IMMEDIATE, true>, LOAD_REG<ZERO_PAGE, true>, LOAD_REG<ZERO_PAGE_XY, true>, LOAD_REG<ABSOLUTE, true>, LOAD_REG<ABSOLUTE_Y, true>},
                        "LDX");

    /// LDY (Load Y Register)
    create_instructions({0xA0, 0xA4, 0xB4, 0xAC, 0xBC},
                        {LOAD_REG<IMMEDIATE, false>, LOAD_REG<ZERO_PAGE, false>, LOAD_REG<ZERO_PAGE_XY, false>, LOAD_REG<ABSOLUTE, false>, LOAD_REG<ABSOLUTE_X, false>},
                        "LDY");

    /// LSR (Logical Shift Right)
    create_instructions({0x4A, 0x46, 0x56, 0x4E, 0x5E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::SHIFT_RIGHT>, BITSHIFT<ZERO_PAGE, Bitshift::SHIFT_RIGHT>, BITSHIFT<ZERO_PAGE_XY, Bitshift::SHIFT_RIGHT>,
                                BITSHIFT<ABSOLUTE, Bitshift::SHIFT_RIGHT>, BITSHIFT<ABSOLUTE_X, Bitshift::SHIFT_RIGHT>},
                                "LSR");

    /// NOP (No Operation)
    create_instructions({0xEA}, {NOP}, "NOP");

    /// ORA (Logical Inclusive OR)
    create_instructions({0x09, 0x05, 0x15, 0x0D, 0x1D, 0x19, 0x01, 0x11},
                        {ORA<IMMEDIATE>, ORA<ZERO_PAGE>, ORA<ZERO_PAGE_XY>, ORA<ABSOLUTE>, ORA<ABSOLUTE_X>, ORA<ABSOLUTE_Y>, ORA<INDIRECT_X>, ORA<INDIRECT_Y>},
                        "ORA");

    /// PHA (Push Accumulator)
    create_instructions({0x48}, {PUSH_REG<true>}, "PHA");

    /// PHP (Push Processor Status)
    create_instructions({0x08}, {PUSH_REG<false>}, "PHP");

    /// PLA (Pull Accumulator)
    create_instructions({0x68}, {PULL_REG<true>}, "PLA");

    /// PLP (Pull Processor Status)
    create_instructions({0x28}, {PULL_REG<false>}, "PLP");

    /// ROL (Rotate Left)
    create_instructions({0x2A, 0x26, 0x36, 0x2E, 0x3E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::ROTATE_LEFT>, BITSHIFT<ZERO_PAGE, Bitshift::ROTATE_LEFT>, BITSHIFT<ZERO_PAGE_XY, Bitshift::ROTATE_LEFT>,
                         BITSHIFT<ABSOLUTE, Bitshift::ROTATE_LEFT>, BITSHIFT<ABSOLUTE_X, Bitshift::ROTATE_LEFT>},
                         "ROL");

    /// ROR (Rotate Right)
    create_instructions({0x6A, 0x66, 0x76, 0x6E, 0x7E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::ROTATE_RIGHT>, BITSHIFT<ZERO_PAGE, Bitshift::ROTATE_RIGHT>, BITSHIFT<ZERO_PAGE_XY, Bitshift::ROTATE_RIGHT>,
                         BITSHIFT<ABSOLUTE, Bitshift::ROTATE_RIGHT>, BITSHIFT<ABSOLUTE_X, Bitshift::ROTATE_RIGHT>},
                         "ROR");

    /// RTI (Return from Interrupt)
    create_instructions({0x40}, {RTI}, "RTI");

    /// RTS (Return from Subroutine)
    create_instructions({0x60}, {RTS}, "RTS");

    // SBC (Subtract with Carry)
    create_instructions({0xE9, 0xE5, 0xF5, 0xED, 0xFD, 0xF9, 0xE1, 0xF1},
                        {SBC<IMMEDIATE>, SBC<ZERO_PAGE>, SBC<ZERO_PAGE_XY>, SBC<ABSOLUTE>, SBC<ABSOLUTE_X>, SBC<ABSOLUTE_Y>, SBC<INDIRECT_X>, SBC<INDIRECT_Y>},
                        "SBC");

    /// SEC (Set Carry Flag)
    create_instructions({0x38}, {FLAGSET<CARRY_FLAG, true>}, "SEC");

    /// SED (Set Decimal Flag)
    create_instructions({0xF8}, {FLAGSET<DECIMAL_FLAG, true>}, "SED");

    /// SEI (Set Interrupt Disable)
    create_instructions({0x78}, {FLAGSET<INTERRUPT_DISABLE_FLAG, true>}, "SEI");

    /// STA (Store Accumulator)
    create_instructions({0x85, 0x95, 0x8D, 0x9D, 0x99, 0x81, 0x91},
                        {STORE_REG<ZERO_PAGE, Register::A>, STORE_REG<ZERO_PAGE_XY, Register::A>, STORE_REG<ABSOLUTE, Register::A>,
                        STORE_REG<ABSOLUTE_X, Register::A>, STORE_REG<ABSOLUTE_Y, Register::A>, STORE_REG<INDIRECT_X, Register::A>,
                        STORE_REG<INDIRECT_Y, Register::A>},
                        "STA");

    /// STX (Store X Register)
    create_instructions({0x86, 0x96, 0x8E},
                        {STORE_REG<ZERO_PAGE, Register::X>, STORE_REG<ZERO_PAGE_XY, Register::X>, STORE_REG<ABSOLUTE, Register::X>},
                        "STX");

    /// STY (Store Y Register)
    create_instructions({0x84, 0x94, 0x8C},
                        {STORE_REG<ZERO_PAGE, Register::Y>, STORE_REG<ZERO_PAGE_XY, Register::Y>, STORE_REG<ABSOLUTE, Register::Y>},
                        "STY");

    /// TAX (Transfer Accumulator to X)
    create_instructions({0xAA}, {TRANSFER_REG<Register::A, Register::X>}, "TAX");

    /// TAY (Transfer Accumulator to Y)
    create_instructions({0xA8}, {TRANSFER_REG<Register::A, Register::Y>}, "TAY");

    /// TSX (Transfer Stack Pointer to X)
    create_instructions({0xBA}, {TRANSFER_REG<Register::SP, Register::X>}, "TSX");

    /// TXA (Transfer X to Accumulator)
    create_instructions({0x8A}, {TRANSFER_REG<Register::X, Register::A>}, "TXA");

    /// TXS (Transfer X to Stack Pointer)
    create_instructions({0x9A}, {TRANSFER_REG<Register::X, Register::SP>}, "TXS");

    /// TYA (Transfer Y to Accumulator)
    create_instructions({0x98}, {TRANSFER_REG<Register::Y, Register::A>}, "TYA");

    auto time_end = std::chrono::steady_clock::now();
    std::cout << "InstructionTable Initialization Completed in " << std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin).count() << " microseconds.\n";
}

InstructionTable::~InstructionTable() {
    std::cout << "InstructionTable Deinitialized!\n";
}
