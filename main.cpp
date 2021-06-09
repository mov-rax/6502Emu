#include <iostream>
#include <bitset>
#include "instruction.hpp"
//#include "cpu.hpp"
using namespace instructions;

uint8_t add(uint8_t a, uint8_t b){
    return a + b;
}

uint8_t sub(uint8_t a, uint8_t b){
    return add(a,~b+1);
}


int main() {
    Cpu cpu;

    union { uint8_t a; int8_t b; };
    union { uint8_t c; int8_t d; };

    a = 4;
    d = -10;
    uint8_t res = sub(a, c);

    printf("unsigned: %u\n", res);
    printf("signed: %d\n", *(int8_t*)&res);

//    cpu.PS.set(0b11111111);
//    //cpu.PS.B = 3;
//    std::bitset<8> bits{cpu.PS.conv()};
//    //printf("VALUE: %X\n", cpu.PS.conv());
//    //cpu.PS.V = 1; cpu.PS.I = 1;
//    std::cout << "yeet: " <<  bits << std::endl;
    return 0;
}
