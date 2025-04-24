#include "instruction_set.h"

size_t get_instruction_size(Instruction inst) {
    switch (inst.opcode) {
        case HLT:
        case NOP:
        case RET:
        case CLI:
        case STI:
            return 1;
        case INT:
        case CALL:
            return 2;
        case PUSH:
        case INC:
        case DEC:
            return 4;
        case MOV:
            return 7;
    }

    return 0;
}

Instruction parse_instruction(CPU *cpu) {
    Instruction inst;

    inst.opcode = cpu->memory[cpu->pc];
    inst.mode1 = cpu->memory[cpu->pc + 1];
    inst.operand1 = cpu->memory[cpu->pc + 2] | (cpu->memory[cpu->pc + 3] << 8);
    inst.mode2 = cpu->memory[cpu->pc + 4];
    inst.operand2 = cpu->memory[cpu->pc + 5] | (cpu->memory[cpu->pc + 6] << 8);

    return inst;
}

int exec_instruction(CPU *cpu, Instruction inst) {
    size_t inst_size = get_instruction_size(inst);

    cpu->ip = inst.opcode;
    printf("inst_opcode = 0x%02x  inst_size = %d\n  inst_mode1 = 0x%02x  inst_op1 = 0x%04x  inst_mode2 = 0x%02x  inst_op2 = 0x%04x\n", inst.opcode, inst_size, inst.mode1, inst.operand1, inst.mode2, inst.operand2);

    switch (inst.opcode) {
        case MOV: {
            if (inst.mode1 != MODE_VAL_IND || (inst.operand1 < R0 || inst.operand1 > R7))
                return 1;
            
            if (inst.mode2 == MODE_VAL_IMM)
                cpu->registers[inst.operand1] = inst.operand2;
            else
                cpu->registers[inst.operand1] = cpu->registers[inst.operand2];

            break;
        }

        case LD: {
            if (inst.mode1 != MODE_VAL_IND || (inst.operand1 < R0 || inst.operand1 > R7) || inst.mode2 != MODE_VAL_IND || (inst.operand2 >= R0 && inst.operand2 <= R7))
                return 1;
            
            cpu->registers[inst.operand1] = cpu->memory[inst.operand2];

            break;
        }

        case HLT: {
            cpu->halted = true;

            break;
        }

        default: {
            return 1;
        }
    }

    cpu->pc += inst_size;

    return 0;
}