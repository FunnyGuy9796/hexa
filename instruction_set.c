#include "instruction_set.h"

bool is_reg(uint16_t val) {
    return val >= R0 && val <= R7;
}

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
        case POP:
        case INC:
        case DEC:
            return 4;
        case MOV:
        case LD:
        case ST:
            return 7;
    }

    return 0;
}

Instruction parse_instruction(CPU *cpu) {
    Instruction inst;

    inst.opcode = cpu->memory[cpu->pc];
    inst.mode1 = cpu->memory[cpu->pc + 1];
    inst.operand1 = (cpu->memory[cpu->pc + 2] << 8) | cpu->memory[cpu->pc + 3];
    inst.mode2 = cpu->memory[cpu->pc + 4];
    inst.operand2 = (cpu->memory[cpu->pc + 5] << 8) | cpu->memory[cpu->pc + 6];

    return inst;
}

int exec_instruction(CPU *cpu, Instruction inst) {
    size_t inst_size = get_instruction_size(inst);

    cpu->ip = inst.opcode;
    printf("inst_opcode = 0x%02x  inst_size = %d\n  inst_mode1 = 0x%02x  inst_op1 = 0x%04x  inst_mode2 = 0x%02x  inst_op2 = 0x%04x\n", inst.opcode, inst_size, inst.mode1, inst.operand1, inst.mode2, inst.operand2);

    switch (inst.opcode) {
        case MOV: {
            bool isSP = false;

            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1)) {
                if (inst.operand1 != SP)
                    return 1;
                else {
                    isSP = true;
                }
            }
            
            if (inst.mode2 == MODE_VAL_IMM) {
                if (isSP)
                    cpu->sp = inst.operand2;
                else
                    cpu->registers[inst.operand1] = inst.operand2;
            } else {
                if (isSP)
                    cpu->sp = cpu->registers[inst.operand2];
                else
                    cpu->registers[inst.operand1] = cpu->registers[inst.operand2];
            }

            break;
        }

        case LD: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1) || inst.mode2 != MODE_VAL_IND || is_reg(inst.operand2))
                return 1;
            
            cpu->registers[inst.operand1] = cpu->memory[inst.operand2];

            break;
        }

        case ST: {
            if (inst.mode1 != MODE_VAL_IND || is_reg(inst.operand1))
                return 1;
            
            cpu->memory[inst.operand1] = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            
            break;
        }

        case PUSH: {
            uint16_t value;

            value = (inst.mode1 == MODE_VAL_IND) ? (is_reg(inst.operand1) ? cpu->registers[inst.operand1] : (cpu->memory[inst.operand1] | cpu->memory[inst.operand1 + 1] << 8)) : inst.operand1;
            
            cpu_push(cpu, value);

            break;
        }

        case POP: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            cpu->registers[inst.operand1] = cpu_pop(cpu);

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