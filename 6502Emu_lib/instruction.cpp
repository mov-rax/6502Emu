#include "instruction.hpp"
#include <chrono>
#include <iostream>
#include <bit>
#include <fmt/format.h>
#include <fmt/color.h>



using namespace addressing;
using namespace utils;

/// BEGIN FLAG-SETTING MACROS

#define CHECK_N_FLAG(val) cpu.PS.N = ((val) & 0x80) >> 7
#define CHECK_Z_FLAG(val) cpu.PS.Z = !(val)

/// END FLAG-SETTING MACROS


std::string Instruction::to_string() const {
    #if ENABLE_INSTRUCTION_DEBUG_INFO
    AddressingMode addr_mode = addrmode_mask(opcode);
    std::string addr_mode_str = (std::string[]){
        "INDIRECT_X",
        "ACC",
        "ABS",
        "INDIRECT",
        "INDIRECT_Y",
        "Z_PAGE",
        "ABS_Y",
        "ABS_X"
    }[(int)addr_mode];
    
    return fmt::format("{} ({})", id, addr_mode_str);
    #else
    return "";
    #endif
}

AddressingMode Instruction::addr_mode() const{
    return addrmode_mask(opcode);
}

#define _D_FMT(args...) fmt::format("${:04X}: {} " args)
#define D_FMT(_fmt, ...) _D_FMT(_fmt, addr, instruction.id __VA_OPT__(,) __VA_ARGS__)

std::string DecompiledInstruction::to_string() const{
    switch (instruction.mode) {
        case INDIRECT_X:
            return D_FMT("(${:02X},X)", raw[1]);
        case ZERO_PAGE:
            return D_FMT("${:02X}", raw[1]);
        case IMMEDIATE:
            return D_FMT("#${:02X}", raw[1]);
        case ACCUMULATOR:
        case IMPLIED:
            return D_FMT("");
        case ABSOLUTE:
            return D_FMT("${:04X}", ((uint16_t)raw[1]) | ((uint16_t)raw[2] << 4));
        case INDIRECT_Y:
            return D_FMT("(${:02X}),Y", raw[1]);
        case ZERO_PAGE_X:
            return D_FMT("${:02X},X", raw[1]);
        case ZERO_PAGE_Y:
            return D_FMT("${:02X},Y", raw[1]);
        case ABSOLUTE_X:
            return D_FMT("${:04X},X", ((uint16_t)raw[1]) | ((uint16_t)raw[2] << 4));
        case ABSOLUTE_Y:
            return D_FMT("${:04X},Y", ((uint16_t)raw[1]) | ((uint16_t)raw[2] << 4));
        case RELATIVE:
            return D_FMT("[${:04X}]", addr + std::bit_cast<int8_t, uint8_t>(raw[1]));
        default:
            return D_FMT("???");
        
    }
}

DecompiledInstruction::DecompiledInstruction(InstructionTable const& table, Mem const& memory, std::size_t addr){
    this->addr = addr;
    instruction = table.get(memory.get(addr));
    raw[0] = memory.get(addr);
    switch (instruction.mode){
        case ABSOLUTE:
        case INDIRECT:
        case ABSOLUTE_X:
        case ABSOLUTE_Y:
        raw[2] = memory.get(addr+2);
        case INDIRECT_X:
        case ZERO_PAGE:
        case IMMEDIATE:
        case INDIRECT_Y:
        case ZERO_PAGE_X:
        case ZERO_PAGE_Y:
        case RELATIVE:
            raw[1] = memory.get(addr+1);
        case ACCUMULATOR:
        case IMPLIED:
            break;
        default:
            fmt::print(fmt::emphasis::bold | fmt::emphasis::italic | fmt::fg(fmt::color::red), 
                "DecompiledInstruction Instatiation Error: Invalid instruction mode {} from {} at address {}\n", instruction.mode, instruction.id, addr);
            abort();
    }
}

/// ADC (Add with carry)
template<AddressingMode mode>
static cycles instructions::ADC(Cpu& cpu){
    constexpr cycles cyc = get_cycles<mode, 8>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},{2,3,4,4,4,4,6,5});
    auto data = load_addr<mode, NORMAL_MODE>(cpu);

    if (cpu.PS.D) { // BCD
        uint8_t lower = (uint8_t)(data.first & 0x0F) + (cpu.A & 0x0F) + cpu.PS.C;
        if (lower > 0x9) lower += 6;

        uint8_t upper = (uint8_t)(data.first >> 4) + (cpu.A >> 4) + (lower > 0x0F);
        if (upper > 0x9) upper += 6;
       
        uint8_t h1 = cpu.A >> 4;
        uint8_t h2 = (uint8_t)data.first >> 4;
        uint8_t s1 = (h1 & 0x8) ? h1 - 0x0F : h1;
        uint8_t s2 = (h2 & 0x8) ? h2 - 0x0F : h2;
        int8_t s = std::bit_cast<int8_t, uint8_t>(s1+s2);
        
        cpu.PS.N = cpu.PS.V = cpu.PS.Z = cpu.PS.C = 0;
        cpu.PS.V = (s < -8 || s > 7);
        cpu.PS.C = (upper > 0xF);
        cpu.A = (upper << 4) | (lower & 0xF);
        cpu.PS.Z = (cpu.A == 0);
        cpu.PS.N = (upper >> 3);
        
    } else {
        uint16_t result = (uint16_t)cpu.A + (uint16_t)data.first + (uint16_t)cpu.PS.C;
        cpu.PS.C = result > 255;
        cpu.PS.N = (result & 0x80) > 0;
        cpu.PS.Z = (result & 0xFF) == 0;
        cpu.PS.V = ((~((uint16_t)cpu.A ^ (uint16_t)data.first)) & ((uint16_t)cpu.A ^ (result)) & 0x80) > 0;
        cpu.A = (uint8_t)(result & 0xFF);
    }

    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(mode) && data.second ? cyc + 1 : cyc;
}

/// AND (logical AND)
template<AddressingMode Mode>
static cycles instructions::AND(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y}, {2, 3, 4, 4, 4, 4, 6, 5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A &= data.first;
    CHECK_Z_FLAG(cpu.A);
    CHECK_N_FLAG(cpu.A);
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

/// ASL (Arithmetic Shift Left)
template<AddressingMode Mode>
static cycles instructions::ASL(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X}, {2, 5, 6, 6, 7});
    std::pair<uint16_t, bool> data;
    uint16_t tmp;
    if (contains_modes<ACCUMULATOR>(Mode)){
        //cpu.PS.C = cpu.A & 0x80;
        tmp = cpu.A << 1;
        cpu.A = tmp;
        //cpu.PS.N = cpu.A & 0x80;
    } else {
        data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
        tmp = cpu.memory.get(data.first);
        cpu.memory.set(data.first, tmp << 1);
    }
    cpu.PS.C = tmp >> 8;
    CHECK_Z_FLAG(tmp);
    CHECK_N_FLAG(tmp);
    return cyc;
}

/// BRANCH (Generic function for all relative branching in the 6502)
template<PSFlagType Flag, bool IsSet>
static cycles instructions::BRANCH(Cpu& cpu){
    int8_t data = std::bit_cast<int8_t, uint8_t>(cpu.memory.get(cpu.PC++));
    uint16_t branch_location = (uint16_t)((int16_t)cpu.PC + (int16_t)data);
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

    CHECK_Z_FLAG(value);
    CHECK_N_FLAG(data.first);
    cpu.PS.V = (data.first & 0x40) >> 6;
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
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.PS.N = ((cpu.A - data.first) & 0x80) > 0;
    cpu.PS.Z = (cpu.A == data.first);
    cpu.PS.C = (cpu.A >= data.first);
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX>
static cycles instructions::CMP_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ABSOLUTE}, {2,3,4});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    if (IsX) {
        cpu.PS.N = ((cpu.X - data.first) & 0x80) > 0;
        cpu.PS.Z = (cpu.X == data.first);
        cpu.PS.C = (cpu.X >= data.first);
    } else {
        cpu.PS.N = ((cpu.Y - data.first) & 0x80) > 0;
        cpu.PS.Z = (cpu.Y == data.first);
        cpu.PS.C = (cpu.Y >= data.first);
    }
    return cyc;
}

template<AddressingMode Mode, bool IsIncrement>
static cycles instructions::INCDEC_MEMORY(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X}, {5,6,6,7});
    auto data = load_addr_ref<Mode, NORMAL_MODE>(cpu);
    uint8_t result = IsIncrement ?  cpu.memory.get(data.first)+1 : cpu.memory.get(data.first)-1;
    CHECK_Z_FLAG(result);
    CHECK_N_FLAG(result);
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
    CHECK_Z_FLAG(result);
    CHECK_N_FLAG(result);
    return cyc;
}

/// EOR (Exclusive OR)
template<AddressingMode Mode>
static cycles instructions::EOR(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A ^= data.first;
    CHECK_Z_FLAG(cpu.A);
    CHECK_N_FLAG(cpu.A);
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc+1 : cyc;
}

/// JMP (Jump)
template<AddressingMode AMode, ModeType MMode>
static cycles instructions::JMP(Cpu& cpu){
    auto data = load_addr_ref<AMode, MMode>(cpu);
    cpu.PC = data.first;
    return (MMode == NORMAL_MODE) ? 3 : 5;
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
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A = data.first;
    CHECK_Z_FLAG(cpu.A);
    CHECK_N_FLAG(cpu.A);
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, bool IsX, ModeType MMode>
static cycles instructions::LOAD_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ZERO_PAGE_Y, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y}, {2,3,4,4,4,4,4});
    auto data = load_addr<Mode, MMode>(cpu);
    uint8_t result;
    if (IsX){ // X Register
        result = (cpu.X = data.first);
    } else { // Y Register
        result = (cpu.Y = data.first);
    }
    CHECK_Z_FLAG(result);
    CHECK_N_FLAG(result);
    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y>(Mode) && data.second ? cyc + 1 : cyc;
}

template<AddressingMode Mode, Bitshift::Enum ShiftType>
static cycles instructions::BITSHIFT(Cpu& cpu){
    uint16_t tmp;
    constexpr cycles cyc = get_cycles<Mode>({ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X}, {2,5,6,6,7});
    if (contains_modes<ACCUMULATOR>(Mode)){
        switch (ShiftType){
            case Bitshift::ROTATE_LEFT:
                tmp = (cpu.A << 1) | cpu.PS.C;
                cpu.PS.C = tmp & 0x100;
                cpu.A = tmp & 0xFF;
                break;
            case Bitshift::ROTATE_RIGHT:
                tmp = (cpu.PS.C << 7) | (cpu.A >> 1);
                cpu.PS.C = cpu.A & 0x01;
                cpu.A = tmp & 0xFF;
                break;
            case Bitshift::SHIFT_RIGHT:
                cpu.PS.C = cpu.A & 1;
                cpu.PS.N = 0;
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
    switch (ShiftType){
        case Bitshift::ROTATE_LEFT:
            tmp = (value << 1) | cpu.PS.C;
            cpu.PS.C = tmp & 0x100;
            value = tmp & 0xFF;
            break;
        case Bitshift::ROTATE_RIGHT:
            tmp = (cpu.PS.C << 7) | (value >> 1);
            cpu.PS.C = value & 0x01;
            value = tmp & 0xFF;
            break;
        case Bitshift::SHIFT_RIGHT:
            cpu.PS.C = value & 1;
            cpu.PS.N = 0;
            value >>= 1;
            break;
    }

    if (!value) cpu.PS.Z = 1;
    if (value & 0x80) cpu.PS.N = 1;
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
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);
    cpu.A |= data.first;
    CHECK_N_FLAG(cpu.A);
    CHECK_Z_FLAG(cpu.A);
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
        CHECK_N_FLAG(cpu.A);
        CHECK_Z_FLAG(cpu.A);
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
    constexpr cycles cyc = get_cycles<Mode>({IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                                            {2,3,4,4,4,4,6,5});
    auto data = load_addr<Mode, NORMAL_MODE>(cpu);

    if (cpu.PS.D) { // BCD
        uint16_t result = cpu.A - data.first - !cpu.PS.C;
        // split upper and lower into separate bytes for calculation.
        uint8_t lower = (cpu.A & 0x0F) - (data.first & 0x0F) - !cpu.PS.C;
        if (lower & 0x80) lower -= 6; // 0x80 - 0x6 = 0xA (make lower digit wrap between 0x0 and 0x9)

        uint8_t upper = (cpu.A >> 4) - (data.first >> 4) - (lower >> 7); // subtract 1 if lower digit overflowed
        if (upper & 0x80) upper -=6; // 0x80 - 0x6 = 0xA (make lower digit wrap between 0x0 and 0x9)

        cpu.PS.N = cpu.PS.V = cpu.PS.Z = cpu.PS.C = 0;
        cpu.PS.V = (((uint16_t)cpu.A ^ data.first) & ((uint16_t)cpu.A ^ result) & 0x80) > 0;
        cpu.PS.C = ((result & 0xFF00) == 0);
        cpu.A = (upper << 4) | (lower & 0xF);
        cpu.PS.Z = (cpu.A == 0);
        cpu.PS.N = (upper >> 7);

    } else {
        uint16_t result = (uint16_t)cpu.A + ((uint16_t)(data.first) ^ 0x00FF) + (uint16_t)cpu.PS.C;
        cpu.PS.C = result > 0xFF;
        cpu.PS.N = (result & 0x80) > 0;
        cpu.PS.Z = (result & 0xFF) == 0;
        cpu.PS.V = ((~((uint16_t)cpu.A ^ (uint16_t)(~data.first))) & ((uint16_t)cpu.A ^ (result)) & 0x80) > 0;
        cpu.A = (uint8_t)(result & 0xFF);
    }

    return contains_modes<ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_Y>(Mode) && data.second ? cyc + 1 : cyc;
}


template<AddressingMode Mode, Register::Enum Reg>
static cycles instructions::STORE_REG(Cpu& cpu){
    constexpr cycles cyc = get_cycles<Mode>({ZERO_PAGE, ZERO_PAGE_X, ZERO_PAGE_Y, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y}, {3,4,4,4,5,5,6,6});
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
                    CHECK_Z_FLAG(cpu.X);
                    CHECK_N_FLAG(cpu.X);
                    break;
                case Register::Y:
                    cpu.Y = cpu.A;
                    CHECK_Z_FLAG(cpu.Y);
                    CHECK_N_FLAG(cpu.Y);
                    break;
                default:
                    throw std::runtime_error(error_words);
            }
            break;
        case Register::X:
            switch (ToReg){
                case Register::A:
                    cpu.A = cpu.X;
                    CHECK_Z_FLAG(cpu.A);
                    CHECK_N_FLAG(cpu.A);
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
            CHECK_Z_FLAG(cpu.A);
            CHECK_N_FLAG(cpu.A);
            break;
        case Register::SP:
            if (ToReg == Register::X)
                cpu.X = cpu.SP;
            else
                throw std::runtime_error(error_words);
            CHECK_Z_FLAG(cpu.X);
            CHECK_N_FLAG(cpu.X);
            break;
    }
    return cyc;
}

#define CREATE_INSTRUCTION(opcode, innies...) table[opcode] = Instruction(innies)

InstructionTable::InstructionTable(){
    using namespace instructions;
    auto time_begin = std::chrono::steady_clock::now();

    static auto invalid_instr = [](Cpu& cpu)->cycles{
        std::cerr << "INVALID INSTRUCTION AT " << std::hex << (int)cpu.PC << "\n";
        return 0;
    };
    
    for (auto& entry : table) {
        #if ENABLE_INSTRUCTION_DEBUG_INFO
        entry = Instruction(invalid_instr, "???", 0xFF, ACCUMULATOR);
        #else
        entry = Instruction(invalid_instr);
        #endif
    }

    /// ADC
    create_instructions(
            {0x69, 0x65, 0x75, 0x6D, 0x7D, 0x79, 0x61, 0x71},
            {ADC<IMMEDIATE>, ADC<ZERO_PAGE>, ADC<ZERO_PAGE_X>, ADC<ABSOLUTE>, ADC<ABSOLUTE_X>, ADC<ABSOLUTE_Y>, ADC<INDIRECT_X>, ADC<INDIRECT_Y>},
            {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
            "ADC");
    /// AND
    create_instructions({0x29, 0x25, 0x35, 0x2D, 0x3D, 0x39, 0x21, 0x31},
                           {AND<IMMEDIATE>, AND<ZERO_PAGE>, AND<ZERO_PAGE_X>, AND<ABSOLUTE>, AND<ABSOLUTE_X>, AND<ABSOLUTE_Y>, AND<INDIRECT_X>, AND<INDIRECT_Y>},
                           {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                           "AND");
    /// ASL
    create_instructions({0x0A, 0x06, 0x16, 0x0E, 0x1E},
                        {ASL<ACCUMULATOR>, ASL<ZERO_PAGE>, ASL<ZERO_PAGE_X>, ASL<ABSOLUTE>, ASL<ABSOLUTE_X>},
                        {ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "ASL");

    
    /// BCC
    //CREATE_INSTRUCTION(0x90, BRANCH<CARRY_FLAG, false>, "BCC");
    create_instructions({0x90}, {BRANCH<CARRY_FLAG, false>}, {RELATIVE}, "BCC");

    /// BCS
    //CREATE_INSTRUCTION(0xB0, BRANCH<CARRY_FLAG, true>, "BCS");
    create_instructions({0xB0}, {BRANCH<CARRY_FLAG, true>}, {RELATIVE}, "BCS");

    /// BEQ
    //CREATE_INSTRUCTION(0xF0, BRANCH<ZERO_FLAG, true>, "BEQ");
    create_instructions({0xF0}, {BRANCH<ZERO_FLAG, true>}, {RELATIVE}, "BEQ");

    /// BIT
    create_instructions({0x24, 0x2C}, {BIT<ZERO_PAGE>, BIT<ABSOLUTE>}, {ZERO_PAGE, ABSOLUTE}, "BIT");

    /// BMI
    create_instructions({0x30}, {BRANCH<NEGATIVE_FLAG, true>}, {RELATIVE}, "BMI");

    /// BNE
    create_instructions({0xD0}, {BRANCH<ZERO_FLAG, false>}, {RELATIVE}, "BNE");

    /// BPL
    create_instructions({0x10}, {BRANCH<NEGATIVE_FLAG, false>}, {RELATIVE}, "BPL");

    /// BRK
    create_instructions({0x00}, {BRK}, {IMPLIED}, "BRK");

    /// BVC (Branch if Overflow Clear)
    create_instructions({0x50}, {BRANCH<OVERFLOW_FLAG, false>}, {RELATIVE}, "BVC");

    /// BVS (Branch if Overflow Set)
    create_instructions({0x70}, {BRANCH<OVERFLOW_FLAG, true>}, {RELATIVE}, "BVS");

    /// CLC (Clear Carry Flag)
    create_instructions({0x18}, {FLAGSET<CARRY_FLAG, false>}, {IMPLIED}, "CLC");

    /// CLD (Clear Decimal Mode)
    create_instructions({0xD8}, {FLAGSET<DECIMAL_FLAG, false>}, {IMPLIED}, "CLD");

    /// CLI (Clear Interrupt Disable)
    create_instructions({0x58}, {FLAGSET<INTERRUPT_DISABLE_FLAG, false>}, {IMPLIED}, "CLI");

    /// CLV (Clear Overflow Flag)
    create_instructions({0xB8}, {FLAGSET<OVERFLOW_FLAG, false>}, {IMPLIED}, "CLV");

    /// CMP (Compare Accumulator)
    create_instructions({0xC9, 0xC5, 0xD5, 0xCD, 0xDD, 0xD9, 0xC1, 0xD1},
                        {CMP<IMMEDIATE>, CMP<ZERO_PAGE>, CMP<ZERO_PAGE_X>, CMP<ABSOLUTE>, CMP<ABSOLUTE_X>, CMP<ABSOLUTE_Y>, CMP<INDIRECT_X>, CMP<INDIRECT_Y>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "CMP");

    /// CPX (Compare X Register)
    create_instructions({0xE0, 0xE4, 0xEC}, 
                        {CMP_REG<IMMEDIATE, true>, CMP_REG<ZERO_PAGE, true>, CMP_REG<ABSOLUTE, true>}, 
                        {IMMEDIATE, ZERO_PAGE, ABSOLUTE}, 
                        "CPX");

    /// CPY (Compare Y Register
    create_instructions({0xC0, 0xC4, 0xCC}, 
                        {CMP_REG<IMMEDIATE, false>, CMP_REG<ZERO_PAGE, false>, CMP_REG<ABSOLUTE, false>}, 
                        {IMMEDIATE, ZERO_PAGE, ABSOLUTE},
                        "CPY");

    /// DEC (Decrement Memory)
    create_instructions( {0xC6, 0xD6, 0xCE, 0xDE},
                         {INCDEC_MEMORY<ZERO_PAGE, false>, INCDEC_MEMORY<ZERO_PAGE_X, false>, INCDEC_MEMORY<ABSOLUTE, false>, INCDEC_MEMORY<ABSOLUTE_X, false>},
                         {ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                         "DEC");

    /// DEX (Decrement X Register)
    create_instructions({0xCA}, {INCDEC_REG<true, false>}, {IMPLIED}, "DEX");

    /// DEY (Decrement Y Register)
    create_instructions({0x88}, {INCDEC_REG<false, false>}, {IMPLIED}, "DEY");

    /// EOR (Exclusive OR)
    create_instructions({0x49, 0x45, 0x55, 0x4D, 0x5D, 0x59, 0x41, 0x51},
                        {EOR<IMMEDIATE>, EOR<ZERO_PAGE>, EOR<ZERO_PAGE_X>, EOR<ABSOLUTE>, EOR<ABSOLUTE_X>, EOR<ABSOLUTE_Y>, EOR<INDIRECT_X>, EOR<INDIRECT_Y>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "EOR");

    /// INC (Increment Memory)
    create_instructions({0xE6, 0xF6, 0xEE, 0xFE}, 
                        {INCDEC_MEMORY<ZERO_PAGE, true>, INCDEC_MEMORY<ZERO_PAGE_X, true>, INCDEC_MEMORY<ABSOLUTE, true>, INCDEC_MEMORY<ABSOLUTE_X, true>},
                        {ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "INC");

    /// INX (Increment X Register)
    create_instructions({0xE8}, {INCDEC_REG<true, true>}, {IMPLIED}, "INX");

    /// INY (Increment Y Register)
    create_instructions({0xC8}, {INCDEC_REG<false, true>}, {IMPLIED}, "INY");

    /// JMP (Jump)
    create_instructions({0x4C, 0x6C}, 
                        {JMP<ABSOLUTE, NORMAL_MODE>, JMP<INDIRECT, ALTERNATIVE_MODE>},
                        {ABSOLUTE, INDIRECT},
                        "JMP");

    /// JSR (Jump to Subroutine)
    create_instructions({0x20}, {JSR<ABSOLUTE>}, {ABSOLUTE}, "JSR");

    /// LDA (Load Accumulator)
    create_instructions({0xA9, 0xA5, 0xB5, 0xAD, 0xBD, 0xB9, 0xA1, 0xB1},
                        {LDA<IMMEDIATE>, LDA<ZERO_PAGE>, LDA<ZERO_PAGE_X>, LDA<ABSOLUTE>, LDA<ABSOLUTE_X>, LDA<ABSOLUTE_Y>, LDA<INDIRECT_X>, LDA<INDIRECT_Y>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "LDA");

    /// LDX (Load X Register)
    create_instructions({0xA2, 0xA6, 0xB6, 0xAE, 0xBE},
                        {LOAD_REG<IMMEDIATE, true>, LOAD_REG<ZERO_PAGE, true>, LOAD_REG<ZERO_PAGE_Y, true, ALTERNATIVE_MODE>, LOAD_REG<ABSOLUTE, true>, LOAD_REG<ABSOLUTE_Y, true>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_Y, ABSOLUTE, ABSOLUTE_Y},
                        "LDX");

    /// LDY (Load Y Register)
    create_instructions({0xA0, 0xA4, 0xB4, 0xAC, 0xBC},
                        {LOAD_REG<IMMEDIATE, false>, LOAD_REG<ZERO_PAGE, false>, LOAD_REG<ZERO_PAGE_X, false>, LOAD_REG<ABSOLUTE, false>, LOAD_REG<ABSOLUTE_X, false>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "LDY");

    /// LSR (Logical Shift Right)
    create_instructions({0x4A, 0x46, 0x56, 0x4E, 0x5E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::SHIFT_RIGHT>, BITSHIFT<ZERO_PAGE, Bitshift::SHIFT_RIGHT>, BITSHIFT<ZERO_PAGE_X, Bitshift::SHIFT_RIGHT>,
                                BITSHIFT<ABSOLUTE, Bitshift::SHIFT_RIGHT>, BITSHIFT<ABSOLUTE_X, Bitshift::SHIFT_RIGHT>},
                        {ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "LSR");

    /// NOP (No Operation)
    create_instructions({0xEA}, {NOP}, {IMPLIED}, "NOP");

    /// ORA (Logical Inclusive OR)
    create_instructions({0x09, 0x05, 0x15, 0x0D, 0x1D, 0x19, 0x01, 0x11},
                        {ORA<IMMEDIATE>, ORA<ZERO_PAGE>, ORA<ZERO_PAGE_X>, ORA<ABSOLUTE>, ORA<ABSOLUTE_X>, ORA<ABSOLUTE_Y>, ORA<INDIRECT_X>, ORA<INDIRECT_Y>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "ORA");

    /// PHA (Push Accumulator)
    create_instructions({0x48}, {PUSH_REG<true>}, {IMPLIED}, "PHA");

    /// PHP (Push Processor Status)
    create_instructions({0x08}, {PUSH_REG<false>}, {IMPLIED}, "PHP");

    /// PLA (Pull Accumulator)
    create_instructions({0x68}, {PULL_REG<true>}, {IMPLIED}, "PLA");

    /// PLP (Pull Processor Status)
    create_instructions({0x28}, {PULL_REG<false>}, {IMPLIED}, "PLP");

    /// ROL (Rotate Left)
    create_instructions({0x2A, 0x26, 0x36, 0x2E, 0x3E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::ROTATE_LEFT>, BITSHIFT<ZERO_PAGE, Bitshift::ROTATE_LEFT>, BITSHIFT<ZERO_PAGE_X, Bitshift::ROTATE_LEFT>,
                         BITSHIFT<ABSOLUTE, Bitshift::ROTATE_LEFT>, BITSHIFT<ABSOLUTE_X, Bitshift::ROTATE_LEFT>},
                        {ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "ROL");

    /// ROR (Rotate Right)
    create_instructions({0x6A, 0x66, 0x76, 0x6E, 0x7E},
                        {BITSHIFT<ACCUMULATOR, Bitshift::ROTATE_RIGHT>, BITSHIFT<ZERO_PAGE, Bitshift::ROTATE_RIGHT>, BITSHIFT<ZERO_PAGE_X, Bitshift::ROTATE_RIGHT>,
                         BITSHIFT<ABSOLUTE, Bitshift::ROTATE_RIGHT>, BITSHIFT<ABSOLUTE_X, Bitshift::ROTATE_RIGHT>},
                        {ACCUMULATOR, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X},
                        "ROR");

    /// RTI (Return from Interrupt)
    create_instructions({0x40}, {RTI}, {IMPLIED}, "RTI");

    /// RTS (Return from Subroutine)
    create_instructions({0x60}, {RTS}, {IMPLIED}, "RTS");

    // SBC (Subtract with Carry)
    create_instructions({0xE9, 0xE5, 0xF5, 0xED, 0xFD, 0xF9, 0xE1, 0xF1},
                        {SBC<IMMEDIATE>, SBC<ZERO_PAGE>, SBC<ZERO_PAGE_X>, SBC<ABSOLUTE>, SBC<ABSOLUTE_X>, SBC<ABSOLUTE_Y>, SBC<INDIRECT_X>, SBC<INDIRECT_Y>},
                        {IMMEDIATE, ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "SBC");

    /// SEC (Set Carry Flag)
    create_instructions({0x38}, {FLAGSET<CARRY_FLAG, true>}, {IMPLIED}, "SEC");

    /// SED (Set Decimal Flag)
    create_instructions({0xF8}, {FLAGSET<DECIMAL_FLAG, true>}, {IMPLIED}, "SED");

    /// SEI (Set Interrupt Disable)
    create_instructions({0x78}, {FLAGSET<INTERRUPT_DISABLE_FLAG, true>}, {IMPLIED}, "SEI");

    /// STA (Store Accumulator)
    create_instructions({0x85, 0x95, 0x8D, 0x9D, 0x99, 0x81, 0x91},
                        {STORE_REG<ZERO_PAGE, Register::A>, STORE_REG<ZERO_PAGE_X, Register::A>, STORE_REG<ABSOLUTE, Register::A>,
                        STORE_REG<ABSOLUTE_X, Register::A>, STORE_REG<ABSOLUTE_Y, Register::A>, STORE_REG<INDIRECT_X, Register::A>,
                        STORE_REG<INDIRECT_Y, Register::A>},
                        {ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT_X, INDIRECT_Y},
                        "STA");

    /// STX (Store X Register)
    create_instructions({0x86, 0x96, 0x8E},
                        {STORE_REG<ZERO_PAGE, Register::X>, STORE_REG<ZERO_PAGE_Y, Register::X>, STORE_REG<ABSOLUTE, Register::X>},
                        {ZERO_PAGE, ZERO_PAGE_Y, ABSOLUTE},
                        "STX");

    /// STY (Store Y Register)
    create_instructions({0x84, 0x94, 0x8C},
                        {STORE_REG<ZERO_PAGE, Register::Y>, STORE_REG<ZERO_PAGE_X, Register::Y>, STORE_REG<ABSOLUTE, Register::Y>},
                        {ZERO_PAGE, ZERO_PAGE_X, ABSOLUTE},
                        "STY");

    /// TAX (Transfer Accumulator to X)
    create_instructions({0xAA}, {TRANSFER_REG<Register::A, Register::X>}, {IMPLIED}, "TAX");

    /// TAY (Transfer Accumulator to Y)
    create_instructions({0xA8}, {TRANSFER_REG<Register::A, Register::Y>}, {IMPLIED}, "TAY");

    /// TSX (Transfer Stack Pointer to X)
    create_instructions({0xBA}, {TRANSFER_REG<Register::SP, Register::X>}, {IMPLIED}, "TSX");

    /// TXA (Transfer X to Accumulator)
    create_instructions({0x8A}, {TRANSFER_REG<Register::X, Register::A>}, {IMPLIED}, "TXA");

    /// TXS (Transfer X to Stack Pointer)
    create_instructions({0x9A}, {TRANSFER_REG<Register::X, Register::SP>}, {IMPLIED}, "TXS");

    /// TYA (Transfer Y to Accumulator)
    create_instructions({0x98}, {TRANSFER_REG<Register::Y, Register::A>}, {IMPLIED}, "TYA");

    auto time_end = std::chrono::steady_clock::now();
    std::cout << "InstructionTable Initialization Completed in " << std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_begin).count() << " microseconds.\n";
}

InstructionTable::~InstructionTable() {
    std::cout << "InstructionTable Deinitialized!\n";
}
