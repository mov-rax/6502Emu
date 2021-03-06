
#ifndef CPU6502
#define CPU6502
#include "types.h"
#include "mem.hpp"
#include <array>

class Cpu{
    using size_t = std::size_t;
public:
    static const unsigned int STACK_PTR_BASE = 0x0100; // lowest memory address of the SP, which ranges from 0x0100 - 0x01FF
    size_t frequency; // Frequency (Hz)
    Mem memory; // Memory (0xFFFF bytes)
    uint8_t A, X, Y, SP; /// Accumulator, Index Register X, Index Register Y, Stack Pointer
    uint16_t PC; // Program Counter
    /// Processor Status (SIGN FLAG, OVERFLOW FLAG, B FLAG, DECIMAL MODE FLAG, INTERRUPT DISABLE FLAG, ZERO FLAG, CARRY FLAG)
    struct {
        uint8_t C : 1; // CARRY
        uint8_t Z : 1; // ZERO
        uint8_t I : 1; // INTERRUPT DISABLE
        uint8_t D : 1; // DECIMAL
        uint8_t B : 2; // B
        uint8_t V : 1; // OVERFLOW
        uint8_t N : 1; // SIGN

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

    // REGISTER RESET METHODS.

    virtual void reset_A();
    virtual void reset_X();
    virtual void reset_Y();
    virtual void reset_SP();
    virtual void reset_PC();

    // PROGRAM STATUS FLAG RESET METHODS.

    virtual void reset_N();
    virtual void reset_V();
    virtual void reset_B();
    virtual void reset_D();
    virtual void reset_I();
    virtual void reset_Z();
    virtual void reset_C();

    /// Sets all register values to 0.
     auto reset_registers() -> void {
        reset_A(); reset_X(); reset_Y(); reset_SP(); reset_PC();
    }
    /// Sets all flags to default values..
    auto reset_flags() -> void {
         reset_N(); reset_V(); reset_B(); reset_D(); reset_I(); reset_Z(); reset_C();
    }

    /// Sends the RESET signal to the 6502
    auto reset() -> void {
        PC = memory.get(0xFFFC) | (memory.get(0xFFFD) << 8);
    }

    Cpu(){
        frequency = 1660000; // DEFAULTS TO NES FREQUENCY
        reset_registers();
        reset_flags();
        memory.reset();
    }

    auto push(uint8_t data) -> void;
    auto pop() -> uint8_t;

    void execute_instruction();
    void print_debug_info() const;

    template<size_t N>
    void program_write(const uint8_t (&data)[N]) {
        for (int i = 0; i < N; i++){
            memory.set(PC + i, data[i]);
        }
    }

};

#endif