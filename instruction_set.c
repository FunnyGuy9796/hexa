#include "instruction_set.h"

bool pc_modified;

bool is_reg(uint16_t val) {
    return val >= R0 && val <= R7;
}

// TODO: Implement dynamic instruction sizes...
size_t get_instruction_size(Instruction inst) {
    return 7;

    switch (inst.opcode) {
        case HLT:
        case NOP:
        case RET:
        case IRET:
        case CLI:
        case STI:
            return 1;
        case INT:
        case PUSH:
        case POP:
        case INC:
        case DEC:
        case NOT:
            return 3;
        case MOV:
        case LD:
        case ST:
        case CALL:
        case JMP:
        case JZ:
        case JNZ:
        case JE:
        case JNE:
        case JL:
        case JLE:
        case JG:
        case JGE:
        case CMP:
        case SHL:
        case SHR:
        case AND:
        case OR:
        case XOR:
        case ADD:
        case SUB:
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

    pc_modified = false;
    cpu->ip = inst.opcode;
    
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
            uint16_t value = (inst.mode1 == MODE_VAL_IND) ? (is_reg(inst.operand1) ? cpu->registers[inst.operand1] : (cpu->memory[inst.operand1] | cpu->memory[inst.operand1 + 1] << 8)) : inst.operand1;
            
            cpu_push(cpu, value);

            break;
        }

        case POP: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            cpu->registers[inst.operand1] = cpu_pop(cpu);

            break;
        }

        case ADD:
        case SUB: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;

            cpu->registers[inst.operand1] = (inst.opcode == ADD) ? cpu->registers[inst.operand1] + value : cpu->registers[inst.operand1] - value;

            break;
        }

        case INC:
        case DEC: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            cpu->registers[inst.operand1] = (inst.opcode == INC) ? cpu->registers[inst.operand1] + 1 : cpu->registers[inst.operand1] - 1;

            break;
        }

        case AND: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;

            cpu->registers[inst.operand1] &= value;

            break;
        }

        case OR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;

            cpu->registers[inst.operand1] |= value;

            break;
        }

        case XOR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;

            cpu->registers[inst.operand1] ^= value;

            break;
        }

        case NOT: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            cpu->registers[inst.operand1] = ~cpu->registers[inst.operand1];

            break;
        }

        case SHL:
        case SHR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;

            cpu->registers[inst.operand1] = (inst.opcode == SHL) ? cpu->registers[inst.operand1] << value : cpu->registers[inst.operand1] >> value;

            break;
        }

        case CMP: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            uint16_t val1 = cpu->registers[inst.operand1];
            uint16_t val2 = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : (cpu->memory[inst.operand2] | cpu->memory[inst.operand2 + 1] << 8)) : inst.operand2;
            
            cpu->flags &= ~(FLAG_EQUAL | FLAG_LESS | FLAG_GREATER | FLAG_ZERO);

            if (val1 == val2)
                cpu->flags |= FLAG_EQUAL | FLAG_ZERO;
            else if (val1 < val2)
                cpu->flags |= FLAG_LESS;
            else if (val1 > val2)
                cpu->flags |= FLAG_GREATER;
            
            break;
        }

        case JMP: {
            uint16_t jmp_addr = inst.operand1;

            cpu->pc = jmp_addr;
            pc_modified = true;

            break;
        }

        case JZ: {
            if (cpu->flags & FLAG_ZERO) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JNZ: {
            if (!(cpu->flags & FLAG_ZERO)) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JE: {
            if (cpu->flags & FLAG_EQUAL) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JNE: {
            if (!(cpu->flags & FLAG_EQUAL)) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JL: {
            if (cpu->flags & FLAG_LESS) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JLE: {
            if (cpu->flags & (FLAG_EQUAL | FLAG_LESS)) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JG: {
            if (cpu->flags & FLAG_GREATER) {
                uint16_t jmp_addr = inst.operand1;

                cpu->pc = jmp_addr;
                pc_modified = true;
            }

            break;
        }

        case JGE: {
            if (cpu->flags & (FLAG_EQUAL | FLAG_GREATER)) {
                uint16_t jmp_addr = inst.operand1;
                
                cpu->pc = jmp_addr;
                pc_modified = true;
            }

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

    if (!pc_modified)
        cpu->pc += inst_size;

    return 0;
}