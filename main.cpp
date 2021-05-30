#include <iostream>
#include <bitset>
#include "instruction.hpp"
//#include "cpu.hpp"

using namespace instructions;

int main() {

    Cpu cpu;
    cpu.PS.set(0b11111111);
    //cpu.PS.B = 3;
    std::bitset<8> bits{cpu.PS.conv()};
    //printf("VALUE: %X\n", cpu.PS.conv());
    //cpu.PS.V = 1; cpu.PS.I = 1;
    std::cout << "yeet: " <<  bits << std::endl;
    return 0;
}
