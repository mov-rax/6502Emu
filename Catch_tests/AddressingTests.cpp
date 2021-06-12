//
// Created by toast on 6/11/21.
//
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE
#include "catch.hpp"
#include <instruction.hpp>


TEST_CASE("INDIRECT_X ADDRESSING", "[AddressingTests]"){
    Cpu cpu;
    cpu.memory.set(0x3A, 0x04);
    cpu.memory.set(0x3B, 0x31);
    cpu.program_write({0xA2, 0xE9, 0xA0, 0x81, 0x8C, 0x04, 0x31, 0xa1, 0x51});
    cpu.execute_instruction(); // LDX #$E9
    REQUIRE(cpu.X == 0xE9);
    cpu.execute_instruction(); // LDY #$81
    REQUIRE(cpu.Y == 0x81);
    cpu.execute_instruction(); // STY $3104
    REQUIRE(cpu.memory.get(0x3104) == cpu.Y);
    cpu.execute_instruction(); // LDA ($51, X)
    REQUIRE(cpu.A == 0x81);
}

TEST_CASE("ZERO_PAGE ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.memory.set(0x56, 0x1D);
    cpu.program_write({0xA5, 0x56});
    cpu.execute_instruction(); // LDA $56
    REQUIRE(cpu.A == 0x1D);
}

TEST_CASE("IMMEDIATE ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.program_write({0xA9, 0x48});
    cpu.execute_instruction(); // LDA #$48
    REQUIRE(cpu.A == 0x48);
}

TEST_CASE("ABSOLUTE ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.memory.set(0x3491, 0x5C);
    cpu.program_write({0xAD, 0x91, 0x34});
    cpu.execute_instruction(); // LDA $3491
    REQUIRE(cpu.A == 0x5C);
}

TEST_CASE("INDIRECT_Y ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.memory.set(0xA4, 0x51);
    cpu.memory.set(0xA5, 0x3F);
    cpu.program_write({0xA0, 0xE9, 0xA2, 0x81, 0x8E, 0x3A, 0x40, 0xB1, 0xA4});
    cpu.execute_instruction(); // LDY #$E9
    REQUIRE(cpu.Y == 0xE9);
    cpu.execute_instruction(); // LDX #$81
    REQUIRE(cpu.X == 0x81);
    cpu.execute_instruction(); // STX $403A
    REQUIRE(cpu.memory.get(0x403A) == cpu.X);
    cpu.execute_instruction(); // LDA ($A4),Y
    REQUIRE(cpu.A == 0x81);
}

TEST_CASE("ZERO_PAGE_X ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.memory.set(0x3A, 0x04);
    cpu.program_write({0xA2, 0xE9, 0xB5, 0x51});
    cpu.execute_instruction(); // LDX #$E9
    REQUIRE(cpu.X == 0xE9);
    cpu.execute_instruction(); // LDA $51,X
    REQUIRE(cpu.A == 0x04);
}

TEST_CASE("ABSOLUTE_Y ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.program_write({0xA2, 0x81, 0xA0, 0xA3, 0x8E, 0x79, 0x22, 0xB9, 0xD6, 0x21});
    cpu.execute_instruction(); // LDX #$81
    REQUIRE(cpu.X == 0x81);
    cpu.execute_instruction(); // LDY #$A3
    REQUIRE(cpu.Y == 0xA3);
    cpu.execute_instruction(); // STX $2279
    REQUIRE(cpu.memory.get(0x2279) == cpu.X);
    cpu.execute_instruction(); // LDA $21D6, Y
    REQUIRE(cpu.A == 0x81);
    // flags
    REQUIRE(cpu.PS.N == 1);
}

TEST_CASE("ABSOLUTE_X ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.program_write({0xA2, 0xA3, 0xA0, 0x81, 0x8C, 0x79, 0x22, 0xBD, 0xD6, 0x21});
    cpu.execute_instruction(); // LDX #$A3
    REQUIRE(cpu.X == 0xA3);
    cpu.execute_instruction(); // LDY #$81
    REQUIRE(cpu.Y == 0x81);
    cpu.execute_instruction(); // STY $2279
    REQUIRE(cpu.memory.get(0x2279) == cpu.Y);
    cpu.execute_instruction(); // LDA $21D6, X
    REQUIRE(cpu.A == 0x81);
    // flags
    REQUIRE(cpu.PS.N == 1);
}

TEST_CASE("ACCUMULATOR ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.program_write({0xA9, 0x01, 0x2A, 0x6A});
    cpu.execute_instruction(); // LDA #$01
    REQUIRE(cpu.A == 0x01);
    cpu.execute_instruction(); // ROL
    REQUIRE(cpu.A == 0x02);
    cpu.execute_instruction(); // ROR
    REQUIRE(cpu.A == 0x01);
}

TEST_CASE("INDIRECT ADDRESSING", "[AddressingTests]") {
    Cpu cpu;
    cpu.memory.set(0x0237, 0x31);
    cpu.memory.set(0x0238, 0x88);
    cpu.program_write({0x6c, 0x37, 0x02});
    cpu.execute_instruction(); // JMP ($0237)
    REQUIRE(cpu.PC == 0x8831);
}