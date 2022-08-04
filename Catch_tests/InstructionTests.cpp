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

TEST_CASE("Logical Operations", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xA9, 0xFF, 0x29, 0xAA, 0x49, 0xAA, 0x09, 0xBB, 0xA2, 0xFC, 0x86, 0x40, 0x24, 0x40});
    cpu.execute_instruction(); // LDA #$FF
    REQUIRE(cpu.A == 0xFF);
    cpu.execute_instruction(); // AND #$AA
    REQUIRE(cpu.A == 0xAA);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // EOR #$AA
    REQUIRE(cpu.A == 0x00);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.Z == 1);
    cpu.execute_instruction(); // ORA #$BB
    REQUIRE(cpu.A == 0xBB);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // LDX #$FC
    REQUIRE(cpu.X == 0xFC);
    cpu.execute_instruction(); // STX $40
    REQUIRE(cpu.memory.get(0x40) == cpu.X);
    cpu.execute_instruction(); // BIT $40
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.V == 1);
    REQUIRE(cpu.PS.C == 0);
}

TEST_CASE("Shift Operations", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xa9, 0xf1, 0x0a, 0x18, 0x4a, 0x38, 0x2a, 0xa9, 0x01, 0x6a, 0x6a});
    cpu.execute_instruction(); // LDA #$F1
    REQUIRE(cpu.A == 0xF1);
    cpu.execute_instruction(); // ASL A
    REQUIRE(cpu.A == 0xE2); 
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // CLC
    REQUIRE(cpu.PS.C == 0);
    cpu.execute_instruction(); // LSR A
    REQUIRE(cpu.A == 0x71);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.N == 0);
    cpu.execute_instruction(); // SEC
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // ROL A
    REQUIRE(cpu.A == 0xE3);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // LDA #$01
    REQUIRE(cpu.A == 0x01);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.N == 0);
    cpu.execute_instruction(); // ROR A
    REQUIRE(cpu.A == 0);
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // ROR A
    REQUIRE(cpu.A == 0x80);
    REQUIRE(cpu.PS.C == 0);
}

TEST_CASE("Decrements and Increments", "[InstructionTests]"){
    Cpu cpu;
    cpu.program_write({0xa9, 0x20, 0x85, 0x10, 0xc6, 0x10, 0xa5, 0x10, 0xca, 0x88, 0xe6, 0x10, 0xa5, 0x10, 0xe8, 0xc8});
    cpu.execute_instruction(); // LDA #$20
    REQUIRE(cpu.A == 0x20);
    cpu.execute_instruction(); // STA $10
    REQUIRE(cpu.memory.get(0x10) == cpu.A);
    cpu.execute_instruction(); // DEC $10
    REQUIRE(cpu.memory.get(0x10) == 0x1F);
    cpu.execute_instruction(); // LDA $10
    REQUIRE(cpu.memory.get(0x10) == cpu.A);
    cpu.execute_instruction(); // DEX
    REQUIRE(cpu.X == 0xFF);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // DEY 
    REQUIRE(cpu.Y == 0xFF);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // INC $10
    REQUIRE(cpu.memory.get(0x10) == 0x20);
    REQUIRE(cpu.PS.N == 0);
    cpu.execute_instruction(); // LDA $10
    REQUIRE(cpu.A == 0x20);
    cpu.execute_instruction(); // INX
    REQUIRE(cpu.X == 0);
    cpu.execute_instruction(); // INY
    REQUIRE(cpu.Y == 0);
}

TEST_CASE("Flag operations", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0x38, 0x18, 0xf8, 0xd8, 0x78, 0x58, 0x38, 0xa9, 0x7f, 0x69, 0x40, 0xb8 });
    cpu.execute_instruction(); // SEC
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // CLC
    REQUIRE(cpu.PS.C == 0);
    cpu.execute_instruction(); // SED
    REQUIRE(cpu.PS.D == 1);
    cpu.execute_instruction(); // CLD
    REQUIRE(cpu.PS.D == 0);
    cpu.execute_instruction(); // SEI
    REQUIRE(cpu.PS.I == 1);
    cpu.execute_instruction(); // CLI
    REQUIRE(cpu.PS.I == 0);
    cpu.execute_instruction(); // SEC
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // LDA #$7F
    REQUIRE(cpu.A == 0x7F);
    cpu.execute_instruction(); // ADC #$40
    REQUIRE(cpu.A == 0xC0);
    REQUIRE(cpu.PS.V == 1);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // CLV
    REQUIRE(cpu.PS.V == 0);
}

TEST_CASE("Comparisons Test", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xa9, 0x42, 0xc9, 0x42, 0xc9, 0x43, 0xc9, 0x41 });
    cpu.execute_instruction(); // LDA #$42
    REQUIRE(cpu.A == 0x42);
    cpu.execute_instruction(); // CMP #$42
    REQUIRE(cpu.PS.Z == 1);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.N == 0);
    cpu.execute_instruction(); // CMP #$43
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // CMP #$41
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.N == 0);
}

TEST_CASE("Branching Test", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xa2, 0x08, 0xca, 0x8e, 0x00, 0x02, 0xe0, 0x03, 0xd0, 0xf8, 0x8e, 0x01, 0x02, 0x00});
    cpu.execute_instruction(); // LDX #$08
    REQUIRE(cpu.X == 0x08);
    auto label = cpu.PC;
    for (int i = cpu.X-1; i >= 3; i--){
        cpu.execute_instruction(); // DEX <-- decrement: (label)
        REQUIRE(cpu.X == i);
        cpu.execute_instruction(); // STX $0200
        REQUIRE(cpu.memory.get(0x0200) == i);
        cpu.execute_instruction(); // CPX #$03
        if (i != 3) {
            REQUIRE(cpu.PS.N == 0);
            REQUIRE(cpu.PS.Z == 0);
            REQUIRE(cpu.PS.C == 1);
            cpu.execute_instruction(); // BNE decrement: (label)
            REQUIRE(cpu.PC == label);
        } else {
            REQUIRE(cpu.PS.N == 0);
            REQUIRE(cpu.PS.Z == 1);
            REQUIRE(cpu.PS.C == 1);
            cpu.execute_instruction(); // BNE decrement: no branch
        }
        
    }
    cpu.execute_instruction(); // STX $0201
    REQUIRE(cpu.memory.get(0x0201) == 0x03);
}

TEST_CASE("Addition Tests", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({
        0xa9, 0x02, 0x69, 0x03, 0xa9, 0x02, 0x38, 0x69, 0x03, 0xa9, 
        0x02, 0x69, 0xfe, 0xa9, 0x02, 0x18, 0x69, 0xfd, 0xa9, 0xfd, 
        0x69, 0x06, 0xa9, 0x7d, 0x69, 0x02
        });
    cpu.execute_instruction(); // LDA #2
    REQUIRE(cpu.A == 0x2);
    cpu.execute_instruction(); // ADC #3
    REQUIRE(cpu.A == 0x5);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.V == 0);
    cpu.execute_instruction(); // LDA #2
    REQUIRE(cpu.A == 0x2);
    cpu.execute_instruction(); // SEC
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // ADC #3
    REQUIRE(cpu.A == 6);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.V == 0);
    cpu.execute_instruction(); // LDA #2
    REQUIRE(cpu.A == 0x2);
    cpu.execute_instruction(); // ADC #254
    REQUIRE(cpu.A == 0);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.Z == 1);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.V == 0);
    cpu.execute_instruction(); // LDA #2
    REQUIRE(cpu.A == 2);
    REQUIRE(cpu.PS.Z == 0);
    cpu.execute_instruction(); // CLC
    REQUIRE(cpu.PS.C == 0);
    cpu.execute_instruction(); // ADC #253
    REQUIRE(cpu.A == 255);
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.V == 0);
    cpu.execute_instruction(); // LDA #253
    REQUIRE(cpu.A == 253);
    cpu.execute_instruction(); // ADC #6
    REQUIRE(cpu.A == 3);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.V == 0);
    cpu.execute_instruction(); // LDA #125
    REQUIRE(cpu.A == 125);
    cpu.execute_instruction(); // ADC #2
    REQUIRE(cpu.A == 128);
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.V == 1);
}

TEST_CASE("BCD Addition Tests", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xf8, 0xa9, 0x99, 0x69, 0x01, 0xa9, 0x79, 0x69, 0x79, 0x69, 0x10});
    cpu.execute_instruction(); // SED
    REQUIRE(cpu.PS.D == 1);
    cpu.execute_instruction(); // LDA #$99
    REQUIRE(cpu.A == 0x99);
    cpu.execute_instruction(); // ADC #$01
    REQUIRE(cpu.A == 0);
    REQUIRE(cpu.PS.Z == 1);
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // LDA #$79
    REQUIRE(cpu.A == 0x79);
    REQUIRE(cpu.PS.Z == 0);
    cpu.execute_instruction(); // ADC #$79
    REQUIRE(cpu.A == 0x59);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.V == 1);
    REQUIRE(cpu.PS.N == 0);
    cpu.execute_instruction(); // ADC #$10
    REQUIRE(cpu.A == 0x70);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.V == 0);
    REQUIRE(cpu.PS.N == 0);
}

TEST_CASE("Subtraction Tests", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xa9, 0x10, 0xe9, 0x10, 0xe9, 0x01, 0xe9, 0x20, 0xe9, 0x78 });
    cpu.execute_instruction(); // LDA #$10
    REQUIRE(cpu.A == 0x10);
    cpu.execute_instruction(); // SBC #$10
    REQUIRE(cpu.A == 0xFF);
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.V == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    cpu.execute_instruction(); // SBC #$01
    REQUIRE(cpu.A == 0xFD);
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.V == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // SBC #$20
    REQUIRE(cpu.A == 0xDD);
    REQUIRE(cpu.PS.N == 1);
    REQUIRE(cpu.PS.V == 0);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // SBC #$78
    REQUIRE(cpu.A == 0x65);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.V == 1);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
}

TEST_CASE("BCD Subtraction Tests", "[InstructionTests]") {
    Cpu cpu;
    cpu.program_write({0xf8, 0x38, 0xa9, 0x10, 0xe9, 0x10, 0xe9, 0x01, 0xe9, 0x20, 0xe9, 0x78});
    cpu.execute_instruction(); // SED
    REQUIRE(cpu.PS.D == 1);
    cpu.execute_instruction(); // SEC
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // LDA #$10
    REQUIRE(cpu.A == 0x10);
    cpu.execute_instruction(); // SBC #$10
    REQUIRE(cpu.A == 0);
    REQUIRE(cpu.PS.Z == 1);
    REQUIRE(cpu.PS.C == 1);
    cpu.execute_instruction(); // SBC #$01
    REQUIRE(cpu.A == 0x99);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 0);
    REQUIRE(cpu.PS.N == 1);
    cpu.execute_instruction(); // SBC #$20
    REQUIRE(cpu.A == 0x78);
    REQUIRE(cpu.PS.Z == 0);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.V == 1);
    cpu.execute_instruction(); // SBC #$78
    REQUIRE(cpu.A == 0);
    REQUIRE(cpu.PS.Z == 1);
    REQUIRE(cpu.PS.C == 1);
    REQUIRE(cpu.PS.N == 0);
    REQUIRE(cpu.PS.V == 0);
}