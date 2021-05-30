//
// Created by toast on 5/28/21.
//
#include <functional>

#ifndef INC_6502EMU_TYPES_H
#define INC_6502EMU_TYPES_H

using cycles = unsigned int;
template<typename T>
using instruction_function = std::function<cycles(T)>;

enum AddressingMode{
    INDIRECT_X      = 0,
    ZERO_PAGE       = 1 << 2,
    IMMEDIATE       = 2 << 2,
    ACCUMULATOR     = 2 << 2,
    ABSOLUTE        = 3 << 2,
    INDIRECT        = 3 << 2, // Used exclusively in JMP
    INDIRECT_Y      = 4 << 2,
    ZERO_PAGE_XY    = 5 << 2,
    ABSOLUTE_Y      = 6 << 2,
    ABSOLUTE_X      = 7 << 2,
};

enum PSFlagType{
    NEGATIVE_FLAG,
    OVERFLOW_FLAG,
    B_FLAGS,
    DECIMAL_FLAG,
    INTERRUPT_DISABLE_FLAG,
    ZERO_FLAG,
    CARRY_FLAG
};

enum ModeType{
    NORMAL_MODE,
    ALTERNATIVE_MODE
};

namespace Bitshift{
    enum Enum{
        ROTATE_LEFT,
        ROTATE_RIGHT,
        SHIFT_RIGHT,
    };
}

struct Instruction;
struct InstructionTable;

#endif //INC_6502EMU_TYPES_H
