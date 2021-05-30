
#ifndef CPU6502
#define CPU6502
#include "types.h"
#include "mem.hpp"

class Cpu{
    using size_t = std::size_t;
public:
    size_t frequency; // Frequency (Hz)
    Mem memory; // Memory (0xFFFF bytes)
    uint8_t A, X, Y, SP; /// Accumulator, Index Register X, Index Register Y, Stack Pointer
    uint16_t PC; // Program Counter
    /// Processor Status (SIGN FLAG, OVERFLOW FLAG, B FLAG, DECIMAL MODE FLAG, INTERRUPT DISABLE FLAG, ZERO FLAG, CARRY FLAG)
    struct {
        uint8_t C : 1;
        uint8_t Z : 1;
        uint8_t I : 1;
        uint8_t D : 1;
        uint8_t B : 2;
        uint8_t V : 1;
        uint8_t N : 1;

        auto conv() const -> uint8_t {
            return C | Z << 1 | I << 2 | D << 3 | B << 4 | V << 6 | N << 7;
        }

        auto set(uint8_t value) -> void {
            C = value & 1;
            Z = (value >> 1) & 1;
            I = (value >> 2) & 1;
            D = (value >> 3) & 1;
            B = (value >> 4) & 3;
            V = (value >> 6) & 1;
            N = (value >> 7) & 1;
        }
    } PS;
    //uint8_t N : 1, V : 1, B : 2, D : 1, I : 1, Z : 1, C : 1;

    /// Sets all register values to 0.
    auto reset_registers() -> void {
        A = 0; X = 0; Y = 0; SP = 0xFF; PC = 0;
    }
    /// Sets all flags to 0.
    auto reset_flags() -> void {
        PS.N = 0; PS.V = 0; PS.B = 0; PS.D = 0; PS.I = 0; PS.Z = 0; PS.C = 0;
    }

    auto get_instr(uint16_t opcode) const -> const Instruction&;

    Cpu(){
        frequency = 1660000; // DEFAULTS TO NES FREQUENCY
        reset_registers();
        reset_flags();
        memory.reset();
    }

    auto push(uint8_t data) -> void;
    auto pop() -> uint8_t;

    auto execute_instruction();
    auto get_address() const -> uint16_t;
};

#endif