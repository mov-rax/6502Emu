#include "cpu.hpp"
#include "instruction.hpp"
#include <iostream>

static const InstructionTable table;

void Cpu::execute_instruction() {
    Instruction instr = table.get(this->memory.get(this->PC++));
    std::cout << "Instruction: " << instr.id << "\n";
    cycles cyc = instr.run(*this);
    std::cout << "Cycles: " << cyc << "\n";
}

auto Cpu::push(uint8_t data) -> void {
    memory.set(data, STACK_PTR_BASE + SP--);
}

auto Cpu::pop() -> uint8_t {
    return memory.get(STACK_PTR_BASE + SP++);
}

void Cpu::reset_A() {
    A = 0;
}

void Cpu::reset_X() {
    X = 0;
}

void Cpu::reset_Y() {
    Y = 0;
}

void Cpu::reset_SP() {
    SP = 0xFD;
}

void Cpu::reset_PC() {
    //PC = 0;
    PC = 0x0600;
}

void Cpu::reset_N() {
    PS.N = 0;
}

void Cpu::reset_V() {
    PS.V = 0;
}

void Cpu::reset_B() {
    PS.B = 0;
}

void Cpu::reset_D() {
    PS.D = 0;
}

void Cpu::reset_I() {
    PS.I = 1;
}

void Cpu::reset_Z() {
    PS.Z = 0;
}

void Cpu::reset_C() {
    PS.C = 0;
}

//          uint8_t C : 1;
//        uint8_t Z : 1;
//        uint8_t I : 1;
//        uint8_t D : 1;
//        uint8_t B : 2;
//        uint8_t V : 1;
//        uint8_t N : 1;

void Cpu::print_debug_info() const {
    std::cout << "REGISTERS:\n";
    printf("A:  %X\t(%d)\n", A, A);
    printf("X:  %X\t(%d)\n", X, X);
    printf("Y:  %X\t(%d)\n", Y, Y);
    printf("PC: %X\t(%d)\n", PC, PC);
    std::cout << "PS FLAGS:\n";
    printf("C: %d\n", PS.C);
    printf("Z: %d\n", PS.Z);
    printf("I: %d\n", PS.I);
    printf("D: %d\n", PS.D);
    printf("B: %d\n", PS.B);
    printf("V: %d\n", PS.V);
    printf("N: %d\n", PS.N);
}
