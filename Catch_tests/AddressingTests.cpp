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
