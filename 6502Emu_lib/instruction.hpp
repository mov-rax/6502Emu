#include <cstdlib>
#include <functional>
#include <algorithm>
#include <string>
#include <array>
#include <utility>
#include <vector>
#include <stdexcept>

#include "cpu.hpp"
#ifndef INSTRUCTION
#define INSTRUCTION

#define ENABLE_INSTRUCTION_DEBUG_INFO 1


struct Instruction{
    instruction_function<Cpu&> func; /// function that takes in references to Cpu and Mem. Returns number of cycles taken to run.
    #if ENABLE_INSTRUCTION_DEBUG_INFO
    std::string id;
    uint8_t opcode;     
    AddressingMode mode;

    Instruction() = default;
    Instruction(instruction_function<Cpu&> func, std::string id, uint8_t opcode, AddressingMode mode):
        func(std::move(func)),id(std::move(id)),opcode(opcode),mode(mode){}
    #else

    Instruction(instruction_function<Cpu&> func)
    #endif


    auto run(Cpu& cpu) const -> cycles{
        return func(cpu);
    }

    std::string to_string() const;
    AddressingMode addr_mode() const;
};

struct InstructionTable{
private:
    // template<int N>
    // auto create_instructions(const int (&codes)[N], const instruction_function<Cpu&> (&funcs)[N], std::string name){
    //     for (int i = 0; i < N; i++){
    //         #if ENABLE_INSTRUCTION_DEBUG_INFO
    //         table[codes[i]] = Instruction(funcs[i], name, codes[i]);
    //         #else
    //         table[codes[i]] = Instruction(funcs[i]);
    //         #endif
    //     }
    // }

    template<int N>
    auto create_instructions(const int (&codes)[N], const instruction_function<Cpu&> (&funcs)[N], const AddressingMode (&addr_modes)[N], std::string name){
        for (int i = 0; i < N; i++){
            #if ENABLE_INSTRUCTION_DEBUG_INFO
            table[codes[i]] = Instruction(funcs[i], name, codes[i], addr_modes[i]);
            #else
            table[codes[i]] = Instruction(funcs[i]);
            #endif
        }
    }

    auto create_instructions(const int code, const instruction_function<Cpu&> func, const AddressingMode mode, const std::string name){
        #if ENABLE_INSTRUCTION_DEBUG_INFO
        table[code] = Instruction(func, name, code, mode);
        #else
        table[code] = Instruction(func);
        #endif
    }
    std::array<Instruction, 0xFF> table;
public:
    InstructionTable();
    ~InstructionTable();

    auto get(std::size_t index) const -> const Instruction&{
        return table.at(index);
    }

};

struct DecompiledInstruction {
    Instruction instruction;
    std::array<uint8_t, 3> raw;
    std::size_t addr;

    DecompiledInstruction(Instruction instruction, std::array<uint8_t, 3> raw) : 
        instruction(std::move(instruction)),raw(std::move(raw)),addr(0){}
    DecompiledInstruction(InstructionTable const& table, Mem const& memory, std::size_t addr);

    std::string to_string() const;
};

namespace addressing{

    namespace utils{
        using cycles = unsigned int;

        constexpr auto addrmode_mask(uint8_t op) -> AddressingMode{
            return static_cast<AddressingMode>(op & 0b00011100);
        }

        template<AddressingMode Mode>
        constexpr bool contains_modes(AddressingMode mode){
            return Mode == mode;
        }

        template<AddressingMode Mode1, AddressingMode Mode2, AddressingMode... Modes>
        constexpr bool contains_modes(AddressingMode mode){
            return mode == Mode1 || contains_modes<Mode2, Modes...>(mode);
        }

        template<PSFlagType Flag, bool IsSet>
        bool is_flag(Cpu& cpu){
            switch (Flag){
                case NEGATIVE_FLAG:
                    return IsSet == cpu.PS.N;
                case OVERFLOW_FLAG:
                    return IsSet == cpu.PS.V;
                case B_FLAGS:
                    return IsSet ? cpu.PS.B : ~cpu.PS.B;
                case DECIMAL_FLAG:
                    return IsSet == cpu.PS.D;
                case INTERRUPT_DISABLE_FLAG:
                    return IsSet == cpu.PS.I;
                case ZERO_FLAG:
                    return IsSet == cpu.PS.Z;
                case CARRY_FLAG:
                    return IsSet == cpu.PS.C;
            }
        }

        template<PSFlagType Flag, bool Value>
        constexpr void set_flag(Cpu& cpu){
            switch (Flag){
                case NEGATIVE_FLAG:
                    cpu.PS.N = Value;
                    break;
                case OVERFLOW_FLAG:
                    cpu.PS.V = Value;
                    break;
                case B_FLAGS:
                    throw std::runtime_error("set_flag is not compatible with PSFlagType B_FLAGS");
                    break;
                case DECIMAL_FLAG:
                    cpu.PS.D = Value;
                    break;
                case INTERRUPT_DISABLE_FLAG:
                    cpu.PS.I = Value;
                    break;
                case ZERO_FLAG:
                    cpu.PS.Z = Value;
                    break;
                case CARRY_FLAG:
                    cpu.PS.C = Value;
            }
        }

        template<AddressingMode Mode, int N>
        constexpr cycles get_cycles(const AddressingMode (&modes)[N], const cycles (&_cycles)[N]){
            for (int i = 0; i < N; i++){
                if (Mode == modes[i])
                    return _cycles[i];
            }
            throw std::runtime_error("'mode' does not exist in 'modes'");
        }

        /** Returns the data from a certain addressing mode, and whether or not a page was crossed during addressing.
         *
         * @tparam Mode The compile-time mode given to a function.
         * @tparam _type Two modes. NORMAL_MODE and ALTERNATIVE_MODE.
         * @tparam GetEffectiveAddress A boolean value used if an address is required instead of a value.
         * @param cpu The CPU. The PC will be advanced to the address before the next instruction.
         * @return The value retrieved given the addressing mode, and whether or not a page file was crossed when retrieving the data.
         */
        template<AddressingMode Mode, ModeType _type, bool GetEffectiveAddress = false>
        constexpr std::pair<uint16_t, bool> load_addr(Cpu& cpu){
            uint16_t temp, temp2;
            if (_type == NORMAL_MODE){
                switch (Mode){
                    case INDIRECT_X:
                        temp = (cpu.memory.get(cpu.PC++) + cpu.X) & 0x00FF; // Get zero-page address + X without carry (0x00FF).
                        temp = cpu.memory.get(temp) | (cpu.memory.get(temp + 1) << 8); // Get address at zero-page address.
                        if (GetEffectiveAddress) return std::pair(temp, false);
                        temp = cpu.memory.get(temp);
                        return std::pair(temp, false);
                    case ZERO_PAGE:
                        temp = cpu.memory.get(cpu.PC++);
                        if (GetEffectiveAddress) return std::pair(temp, false);
                        temp = cpu.memory.get(temp);
                        return std::pair(temp, false);
                    case IMMEDIATE:
                        if (GetEffectiveAddress) return std::pair(++cpu.PC, false);
                        temp = cpu.memory.get(cpu.PC++);
                        return std::pair(temp, false);
                    case ABSOLUTE:
                        cpu.PC += 2;
                        if (GetEffectiveAddress) return std::pair(cpu.memory.get(cpu.PC - 2) | (cpu.memory.get(cpu.PC - 1) << 8), false);
                        temp = cpu.memory.get(cpu.memory.get(cpu.PC - 2) | (cpu.memory.get(cpu.PC - 1) << 8)); // 16-bit address
                        return std::pair(temp, false);
                    case INDIRECT_Y:
                        temp = cpu.memory.get(cpu.PC++); // Deref zero-page address
                        temp2 = cpu.memory.get(temp);
                        temp2 = ((temp2 + cpu.Y) & 0x0100) >> 8; // Used to check if page has been crossed or has a carry bit.
                        temp = ((cpu.memory.get(temp) + cpu.Y) & 0x00FF) | ((cpu.memory.get(temp + 1) + temp2) << 8); // Address calculated from ($aa), Y
                        if (GetEffectiveAddress)
                            return std::pair(temp, temp2);
                        return std::pair(cpu.memory.get(temp) | (cpu.memory.get(temp + 1) << 8), temp2);
                    case ZERO_PAGE_X:
                        temp = cpu.memory.get(cpu.PC++); // zero-page address
                        temp = 0x00FF & (temp + cpu.X); // indexed zero-page address ( Adds X to $aaaa without carry )
                        if (GetEffectiveAddress) return std::pair(temp, false);
                        return std::pair(cpu.memory.get(temp), false); // Returns byte at indexed zero-page address.
                    case ABSOLUTE_Y:
                        cpu.PC += 2;
                        temp = cpu.memory.get(cpu.PC - 2) | (cpu.memory.get(cpu.PC - 1) << 8); // 16-bit address
                        temp2 =  ((temp + cpu.Y) & 0xFF00) ^ (temp & 0xFF00); // Checks if page is crossed when indexing.
                        if (GetEffectiveAddress) return std::pair(temp + cpu.Y, temp2);
                        return std::pair(cpu.memory.get(temp+cpu.Y), temp2);
                    case ABSOLUTE_X:
                        cpu.PC += 2;
                        temp = cpu.memory.get(cpu.PC - 2) | (cpu.memory.get(cpu.PC - 1) << 8); // 16-bit address
                        temp2 =  ((temp + cpu.X) & 0xFF00) ^ (temp & 0xFF00); // Checks if page is crossed when indexing.
                        if (GetEffectiveAddress) return std::pair(temp + cpu.X, temp2);
                        return std::pair(cpu.memory.get(temp+cpu.X), temp2);
                    default:
                        throw std::runtime_error("Invalid Mode in NormalMode");
                }
            } else {
                switch (Mode){
                    case ACCUMULATOR: // instruction is 1-byte, therefore nothing should be returned.
                        return std::pair(0, false);
                    case INDIRECT: // Only used in JMP instruction.
                        temp = cpu.memory.get(cpu.PC) | (cpu.memory.get(cpu.PC + 1) << 8);
                        temp = cpu.memory.get(temp) | (cpu.memory.get(temp + 1) << 8); // 16-bit address
                        return std::pair(temp, false); // Return addr -> addr.
                    case ZERO_PAGE_Y:
                        temp = cpu.memory.get(cpu.PC++); // zero-page address
                        temp = 0x00FF & (temp + cpu.Y); // indexed zero-page address ( Adds Y to $aaaa without carry )
                        if (GetEffectiveAddress) return std::pair(temp, false);
                        return std::pair(cpu.memory.get(temp), false); // Returns byte at indexed zero-page address.
                    default:
                        throw std::runtime_error("Invalid Mode in _type");
                }
            }
        }

        template<AddressingMode Mode, ModeType _type>
        constexpr std::pair<uint16_t, bool> load_addr_ref(Cpu& cpu){
            return load_addr<Mode, _type, true>(cpu);
        }

    }
}

namespace instructions{
    using namespace addressing;

    constexpr auto get_instruction(uint8_t opcode){
        AddressingMode f = utils::addrmode_mask(opcode);
    }

        template<AddressingMode Mode>
        static cycles ADC(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles AND(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles ASL(Cpu& cpu);

        template<PSFlagType Flag, bool IsSet>
        static cycles BRANCH(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles BIT(Cpu &cpu);

        static cycles BRK(Cpu &cpu);

        template<PSFlagType Flag, bool Value>
        static cycles FLAGSET(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles CMP(Cpu& cpu);

        template<AddressingMode Mode, bool IsX>
        static cycles CMP_REG(Cpu& cpu);

        template<AddressingMode Mode, bool IsIncrement>
        static cycles INCDEC_MEMORY(Cpu& cpu);

        template<bool IsX, bool IsIncrement>
        static cycles INCDEC_REG(Cpu& cpu);

        template<AddressingMode AMode, ModeType MMode>
        static cycles JMP(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles JSR(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles LDA(Cpu& cpu);

        template<AddressingMode Mode, bool IsX, ModeType MMode = NORMAL_MODE>
        static cycles LOAD_REG(Cpu& cpu);

        template<AddressingMode Mode, Bitshift::Enum ShiftType>
        static cycles BITSHIFT(Cpu& cpu);

        static cycles NOP(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles ORA(Cpu& cpu);

        template<bool IsAcc>
        static cycles PUSH_REG(Cpu& cpu);

        template<bool IsAcc>
        static cycles PULL_REG(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles EOR(Cpu& cpu);

        static cycles RTI(Cpu& cpu);

        static cycles RTS(Cpu& cpu);

        template<AddressingMode Mode>
        static cycles SBC(Cpu& cpu);

        template<AddressingMode Mode, Register::Enum Reg>
        static cycles STORE_REG(Cpu& cpu);

        template<Register::Enum FromReg, Register::Enum ToReg>
        static cycles TRANSFER_REG(Cpu& cpu);
}



#endif