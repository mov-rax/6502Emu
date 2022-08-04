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
    INDIRECT_X,
    ZERO_PAGE,
    IMMEDIATE,
    ACCUMULATOR,
    ABSOLUTE,
    INDIRECT, // Used exclusively in JMP
    INDIRECT_Y ,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    ABSOLUTE_Y,
    ABSOLUTE_X,
    RELATIVE,
    IMPLIED,       
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

namespace Register{
    enum Enum {
        A,
        X,
        Y,
        SP
    };
}

struct Instruction;
struct InstructionTable;

#endif //INC_6502EMU_TYPES_H
