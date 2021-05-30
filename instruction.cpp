#include "instruction.hpp"
#include <chrono>
#include <iostream>
#include <bit>


using namespace addressing;
using namespace utils;

template<AddressingMode mode>
cycles instructions::TEST(Cpu& cpu){
    constexpr cycles cyc = get_cycles<2, mode>({INDIRECT_X, IMMEDIATE}, {3, 2});
    constexpr bool is_mode = contains_modes<IMMEDIATE, INDIRECT_X, ZERO_PAGE>(mode);
    return contains_modes<IMMEDIATE, INDIRECT_X, ZERO_PAGE>(mode) ? cyc + 1 : cyc;
}

/// ADC (Add with carry)
template<AddressingMode mode>
cycles instructions::ADC(Cpu& cpu){
    constexpr cycles cyc = get_cycles<mode, 8>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},{2,3,4,4,4,4,6,5});
    auto data = load_addr<mode, NORMAL_MODE>(cpu);
    uint16_t temp = cpu.A;
    temp += data.first;
    if (temp & 0x0100) { // set both overflow and carry
        cpu.PS.V = 1;
        cpu.PS.C = 1;
    }
    if (temp & 0x0080) cpu.PS.N = 1;
    if (!(temp & 0x00FF)) cpu.PS.Z = 1;
    cpu.A = temp;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(mode) && data.second ? cyc + 1 : cyc;
}

/// AND (logical AND)
template<AddressingMode Mode>
cycles instructions::AND(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y}, {2, 3, 4, 4, 4, 4, 6, 5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A &= data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

/// ASL (Arithmetic Shift Left)
template<AddressingMode Mode>
cycles instructions::ASL(Cpu& cpu){
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
cycles instructions::BRANCH(Cpu& cpu){
    uint16_t branch_location = cpu.PC + (int16_t)cpu.memory.get(cpu.PC++);
    if (is_flag<Flag, IsSet>(cpu)){
        cpu.PC = branch_location;
        if ((branch_location & 0xFF00) ^ (cpu.PC & 0xFF00)) // Check if to a new page.
            return 4;
        else
            return 3;
    }
    return 2;
}

/// BIT (Bit Test)
template<AddressingMode Mode>
cycles instructions::BIT(Cpu &cpu){
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
cycles instructions::BRK(Cpu &cpu){
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
cycles instructions::FLAGSET(Cpu& cpu){
    constexpr cycles cyc = 2;
    cpu.PC++;
    set_flag<Flag,Value>(cpu);
    return cyc;
}

template<AddressingMode Mode>
cycles instructions::CMP(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    if (cpu.A >= data.first) cpu.PS.C = 1;
    if (cpu.A == data.first) cpu.PS.Z = 1;
    if (data.first & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX>
cycles instructions::CMP_REG(Cpu& cpu){
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
cycles instructions::INCDEC_MEMORY(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X}, {5,6,6,7});
    auto data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
    uint8_t result = IsIncrement ?  cpu.memory.get(data.first)+1 : cpu.memory.get(data.first)-1;
    if (!result) cpu.PS.Z = 1;
    if (result & 0x80) cpu.PS.N = 1;
    cpu.memory.set(data.first, result); // decrements
    return cyc;
}

template<bool IsX, bool IsIncrement>
cycles instructions::INCDEC_REG(Cpu& cpu){
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
cycles instructions::EOR(Cpu& cpu){
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
cycles instructions::JMP(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ABSOLUTE, INDIRECT}, {3,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.PC = data.first;
    return cyc;
}

/// JSR (Jump to Subroutine)
template<AddressingMode Mode>
cycles instructions::JSR(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ABSOLUTE}, {6}); // Done to ensure only ABSOLUTE is used in this instruction
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.push(cpu.PC >> 8);
    cpu.push((cpu.PC & 0x00FF) - 1);
    cpu.PC = data.first;
    return cyc;
}

template<AddressingMode Mode>
cycles instructions::LDA(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A = data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX>
cycles instructions::LOAD_REG(Cpu& cpu){
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
cycles instructions::BITSHIFT(Cpu& cpu){
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
cycles instructions::NOP(Cpu& cpu){
    constexpr cycles cyc = 2; // IMPLIED
    cpu.PC++;
    return cyc;
}

/// ORA (Logical Inclusive OR)
template<AddressingMode Mode>
cycles instructions::ORA(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_XY, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A |= data.first;
    if (!cpu.A) cpu.PS.Z = 1;
    if (cpu.A & 0x80) cpu.PS.N = 1;
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<bool IsAcc>
cycles instructions::PUSH_REG(Cpu& cpu){
    constexpr cycles cyc = 3; // IMPLIED
    if (IsAcc)
        cpu.push(cpu.A);
    else
        cpu.push(cpu.PS.conv());
    return cyc;
}


template<bool IsAcc>
cycles instructions::PULL_REG(Cpu& cpu){
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

    auto time_end = std::chrono::steady_clock::now();
    std::cout << "InstructionTable Initialization Completed in " << std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin).count() << " microseconds.\n";
}

InstructionTable::~InstructionTable() {
    std::cout << "InstructionTable Deinitialized!\n";
}
