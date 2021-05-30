#include "cpu.hpp"
#include "instruction.hpp"

static const InstructionTable table;

auto Cpu::execute_instruction() {
//    const instructions::Instruction& instr = instructions::instr_table.get(memory.get(PC));
//    instr.run(*this);
}

auto Cpu::get_address() const -> uint16_t{
    return 3;
}

auto Cpu::push(uint8_t data) -> void {
    memory.set(data, SP--);
}

auto Cpu::pop() -> uint8_t {
    return memory.get(SP++);
}

auto Cpu::get_instr(uint16_t opcode) const -> const Instruction& {
    return table.get(opcode);
}
