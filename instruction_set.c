#include "instruction_set.h"

bool pc_modified;

bool is_reg(uint16_t val) {
    return val >= R0 && val <= R7;
}

size_t get_instruction_size(Instruction inst) {
    return 7;
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
            bool isSP, isPC = false;

            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1)) {
                if (inst.operand1 == SP)
                    isSP = true;
                else if (inst.operand1 == PC)
                    isPC = true;
                else
                    return 1;
            }
            
            if (inst.mode2 == MODE_VAL_IMM) {
                if (isSP)
                    cpu->sp = inst.operand2;
                else if (isPC)
                    cpu->pc = inst.operand2;
                else
                    cpu->registers[inst.operand1] = inst.operand2;
            } else {
                if (isSP)
                    cpu->sp = cpu->registers[inst.operand2];
                else if (isPC)
                    cpu->pc = cpu->registers[inst.operand2];
                else
                    cpu->registers[inst.operand1] = cpu->registers[inst.operand2];
            }

            break;
        }

        case LD: {
            uint16_t value = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];

            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 1;
            
            cpu->registers[inst.operand1] = cpu->memory[value] | (cpu->memory[value + 1] << 8);

            break;
        }

        case ST: {
            uint16_t dest = (inst.mode1 == MODE_VAL_IMM) ? inst.operand1 : cpu->registers[inst.operand1];
            uint16_t value = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];

            cpu->memory[dest] = value & 0xff;
            cpu->memory[dest + 1] = (value >> 8) & 0xff;

            if (dest == SERIAL_DATA)
                cpu->memory[SERIAL_STATUS] |= SERIAL_STATUS_NEW_DATA;
            
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

        case CALL: {
            uint16_t return_addr = cpu->pc + inst_size;
            uint16_t call_addr = inst.operand1;

            cpu_push(cpu, return_addr);
            cpu->pc = call_addr;
            pc_modified = true;

            break;
        }

        case RET: {
            cpu->pc = cpu_pop(cpu);
            pc_modified = true;

            break;
        }

        case IRET: {
            uint16_t int_num = cpu_pop(cpu);

            cpu->flags = cpu_pop(cpu);
            cpu->pc = cpu_pop(cpu);
            pc_modified = true;

            break;
        }

        case CLI: {
            cpu->interrupts_enabled = false;

            break;
        }

        case STI: {
            cpu->interrupts_enabled = true;

            break;
        }

        case NOP: {
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