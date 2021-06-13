//
// Created by toast on 6/12/21.
//

#include "catch.hpp"
#include <instruction.hpp>

/*
 * Only the underlying instruction is tested, not the addressing modes. Therefore, only one addressing mode will be used
 * for each test case. Due to previous testing in AddressingTests.cpp, if the instruction works with ONE addressing mode,
 * then it will work for ALL addressing modes.
*/

TEST_CASE("Load/Store Operations", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xA9, 0x10, 0xA2, 0x20, 0xA0, 0x30, 0x85, 0x10, 0x86, 0x20, 0x84, 0x30});
    cpu.execute_instruction(); // LDA #$10
    REQUIRE(cpu.A == 0x10);
    cpu.execute_instruction(); // LDX #$20
    REQUIRE(cpu.X == 0x20);
    cpu.execute_instruction(); // LDY #$30
    REQUIRE(cpu.Y == 0x30);
    cpu.execute_instruction(); // STA $10
    REQUIRE(cpu.memory.get(0x10) == cpu.A);
    cpu.execute_instruction(); // STX $20
    REQUIRE(cpu.memory.get(0x20) == cpu.X);
    cpu.execute_instruction(); // STY $30
    REQUIRE(cpu.memory.get(0x30) == cpu.Y);
}

TEST_CASE("Register Transfers", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xA9, 0x10, 0xAA, 0xA8, 0xA9, 0x00, 0x8A, 0xA9, 0x00, 0x98});
    cpu.execute_instruction(); // LDA #$10
    REQUIRE(cpu.A == 0x10);
    cpu.execute_instruction(); // TAX
    REQUIRE(cpu.X == cpu.A);
    cpu.execute_instruction(); // TAY
    REQUIRE(cpu.Y == cpu.A);
    cpu.execute_instruction(); // LDA #$00
    REQUIRE(cpu.A == 0x00);
    cpu.execute_instruction(); // TXA
    REQUIRE(cpu.A == cpu.X);
    cpu.execute_instruction(); // LDA #$00
    REQUIRE(cpu.A == 0x00);
    cpu.execute_instruction(); // TYA
    REQUIRE(cpu.A == cpu.Y);
}

TEST_CASE("Stack Operations", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xBA, 0xA2, 0xF0, 0x9A, 0xA9, 0x13, 0x48, 0x08, 0x68, 0x28});
    cpu.execute_instruction(); // TSX
    REQUIRE(cpu.X == cpu.SP);
    cpu.execute_instruction(); // LDX #$F0
    REQUIRE(cpu.X == 0xF0);
    cpu.execute_instruction(); // TXS
    REQUIRE(cpu.SP == cpu.X);
    cpu.execute_instruction(); // LDA #$13
    REQUIRE(cpu.A == 0x13);
    cpu.execute_instruction(); // PHA
    REQUIRE(cpu.memory.get(Cpu::STACK_PTR_BASE + cpu.SP + 1) == cpu.A);
    cpu.execute_instruction(); // PHP
    REQUIRE(cpu.memory.get(Cpu::STACK_PTR_BASE + cpu.SP + 1) == cpu.PS.conv());
    cpu.execute_instruction(); // PLA
    REQUIRE(cpu.A == cpu.PS.conv());
    cpu.execute_instruction(); // PLP
    REQUIRE(cpu.PS.conv() == cpu.memory.get(cpu.STACK_PTR_BASE + cpu.SP));
}
