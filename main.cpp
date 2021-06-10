#include <iostream>
#include <bitset>
#include "instruction.hpp"
//#include "cpu.hpp"
using namespace instructions;


int main() {
    Cpu cpu;
    unsigned char ins[] = {0xA9, 0x05, 0x69, 0x17, 0x00};
    for (int i = 0; i < sizeof(ins)/sizeof(ins[0]); i++){
        cpu.memory.set(cpu.PC + i, ins[i]);
    }
    cpu.execute_instruction();
    cpu.print_debug_info();
    cpu.execute_instruction();
    cpu.print_debug_info();
    return 0;
}
