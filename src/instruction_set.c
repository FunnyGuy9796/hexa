#include "instruction_set.h"
#include "disk.h"

bool pc_modified;
bool framebuffer_dirty = false;

bool is_reg(uint16_t val) {
    return val >= R0 && val <= R7;
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
    pc_modified = false;
    cpu->ip = inst.opcode;

    if (cpu->pc >= BIOS_ADDR)
        cpu->flags &= ~FLAG_RESET;

    if ((cpu->ip == INT || cpu->ip == IRET) && (cpu->flags & FLAG_USER_MODE))
        return 5;

    if ((!(cpu->flags & FLAG_RESET) && (cpu->pc < START_ADDR)) || ((cpu->flags & FLAG_USER_MODE) && (cpu->pc >= BIOS_ADDR)))
        return 4;
    
    switch (inst.opcode) {
        case MOV: {
            bool isSP = false, isPC = false, isCS = false, isSS = false, isDS = false, isUS = false, isFLAGS = false;

            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1)) {
                switch (inst.operand1) {
                    case SP: {
                        isSP = true;

                        break;
                    }

                    case PC: {
                        isPC = true;

                        break;
                    }

                    case CS: {
                        isCS = true;

                        break;
                    }

                    case SS: {
                        isSS = true;

                        break;
                    }

                    case DS: {
                        isDS = true;

                        break;
                    }

                    case US: {
                        isUS = true;

                        break;
                    }

                    case FLAGS: {
                        isFLAGS = true;

                        break;
                    }

                    default:
                        return 2;
                }
            }

            if (isSP)
                cpu->sp = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isPC)
                cpu->pc = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isCS)
                cpu->cs = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isSS)
                cpu->ss = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isDS)
                cpu->ds = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isUS)
                cpu->us = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else if (isFLAGS)
                cpu->flags = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            else
                cpu->registers[inst.operand1] = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];

            break;
        }

        case LD: {
            uint16_t offset = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];

            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr = seg_offset(cpu->ds, offset);

            if (phys_addr % 2 != 0)
                return 3;
            
            cpu->registers[inst.operand1] = (cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1];

            break;
        }

        case ST: {
            uint16_t offset = (inst.mode1 == MODE_VAL_IMM) ? inst.operand1 : cpu->registers[inst.operand1];
            uint16_t value = (inst.mode2 == MODE_VAL_IMM) ? inst.operand2 : cpu->registers[inst.operand2];
            uint32_t phys_addr = seg_offset(cpu->ds, offset);

            if (phys_addr >= BIOS_ADDR)
                return 4;

            if (phys_addr % 2 != 0)
                return 3;

            cpu->memory[phys_addr] = (value >> 8) & 0xff;
            cpu->memory[phys_addr + 1] = value & 0xff;

            if (phys_addr == SERIAL_DATA)
                cpu->memory[SERIAL_STATUS] |= SERIAL_STATUS_NEW_DATA;
            
            if (phys_addr == DISK_COMMAND) {
                if (value == DISK_CMD_WRITE)
                    write_disk(cpu);
                else if (value == DISK_CMD_READ)
                    read_disk(cpu);
            }
            
            if (phys_addr >= FRAMEBUFFER_ADDR && phys_addr <= FRAMEBUFFER_ADDR + (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT))
                framebuffer_dirty = true;
            
            break;
        }

        case PUSH: {
            uint16_t value;

            if (inst.mode1 == MODE_VAL_IND) {
                if (!is_reg(inst.operand1))
                    switch (inst.operand1) {
                        case CS: {
                            value = cpu->cs;

                            break;
                        }

                        case SS: {
                            value = cpu->ss;

                            break;
                        }

                        case DS: {
                            value = cpu->ds;

                            break;
                        }

                        case US: {
                            value = cpu->us;

                            break;
                        }

                        case FLAGS: {
                            value = cpu->flags;

                            break;
                        }

                        default:
                            return 2;
                    }
                else
                    value = cpu->registers[inst.operand1];
            } else
                value = inst.operand1;

            cpu_push(cpu, value);

            break;
        }

        case POP: {
            if (inst.mode1 != MODE_VAL_IND)
                return 2;
            
            bool isCS = false, isSS = false, isDS = false, isUS = false, isFLAGS = false;
            
            if (!is_reg(inst.operand1)) {
                switch (inst.operand1) {
                    case CS: {
                        isCS = true;

                        break;
                    }

                    case SS: {
                        isSS = true;

                        break;
                    }

                    case DS: {
                        isDS = true;

                        break;
                    }

                    case US: {
                        isUS = true;

                        break;
                    }

                    case FLAGS: {
                        isFLAGS = true;

                        break;
                    }

                    default:
                        return 2;
                }
            }
            
            if (isCS)
                cpu->cs = cpu_pop(cpu);
            else if (isSS)
                cpu->ss = cpu_pop(cpu);
            else if (isDS)
                cpu->ds = cpu_pop(cpu);
            else if (isUS)
                cpu->us = cpu_pop(cpu);
            else if (isFLAGS)
                cpu->flags = cpu_pop(cpu);
            else
                cpu->registers[inst.operand1] = cpu_pop(cpu);

            break;
        }

        case ADD:
        case SUB: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }

            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;

            cpu->registers[inst.operand1] = (inst.opcode == ADD) ? cpu->registers[inst.operand1] + value : cpu->registers[inst.operand1] - value;

            break;
        }

        case INC:
        case DEC: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            cpu->registers[inst.operand1] = (inst.opcode == INC) ? cpu->registers[inst.operand1] + 1 : cpu->registers[inst.operand1] - 1;

            break;
        }

        case AND: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;

            cpu->registers[inst.operand1] &= value;

            break;
        }

        case OR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }

            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;

            cpu->registers[inst.operand1] |= value;

            break;
        }

        case XOR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;

            cpu->registers[inst.operand1] ^= value;

            break;
        }

        case NOT: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            cpu->registers[inst.operand1] = ~cpu->registers[inst.operand1];

            break;
        }

        case SHL:
        case SHR: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }
            
            uint16_t value = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;

            cpu->registers[inst.operand1] = (inst.opcode == SHL) ? cpu->registers[inst.operand1] << value : cpu->registers[inst.operand1] >> value;

            break;
        }

        case CMP: {
            if (inst.mode1 != MODE_VAL_IND || !is_reg(inst.operand1))
                return 2;
            
            uint32_t phys_addr;

            if (inst.mode2 == MODE_VAL_IND && !is_reg(inst.operand2)) {
                uint16_t offset = inst.operand2;
                phys_addr = seg_offset(cpu->ds, offset);

                if (phys_addr >= BIOS_ADDR || phys_addr < START_ADDR)
                    return 4;

                if (phys_addr % 2 != 0)
                    return 3;
            }
            
            uint16_t val1 = cpu->registers[inst.operand1];
            uint16_t val2 = (inst.mode2 == MODE_VAL_IND) ? (is_reg(inst.operand2) ? cpu->registers[inst.operand2] : ((cpu->memory[phys_addr] << 8) | cpu->memory[phys_addr + 1])) : inst.operand2;
            
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
            uint16_t offset = inst.operand1;
            uint16_t segment;

            if (cpu->flags & FLAG_USER_MODE)
                segment = cpu->us;
            else
                segment = cpu->cs;
            
            cpu->pc = seg_offset(segment, offset);
            pc_modified = true;

            break;
        }

        case JZ: {
            if (cpu->flags & FLAG_ZERO) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JNZ: {
            if (!(cpu->flags & FLAG_ZERO)) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;
                
                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JE: {
            if (cpu->flags & FLAG_EQUAL) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JNE: {
            if (!(cpu->flags & FLAG_EQUAL)) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JL: {
            if (cpu->flags & FLAG_LESS) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JLE: {
            if (cpu->flags & (FLAG_EQUAL | FLAG_LESS)) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JG: {
            if (cpu->flags & FLAG_GREATER) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;

                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case JGE: {
            if (cpu->flags & (FLAG_EQUAL | FLAG_GREATER)) {
                uint16_t offset = inst.operand1;
                uint16_t segment;

                if (cpu->flags & FLAG_USER_MODE)
                    segment = cpu->us;
                else
                    segment = cpu->cs;
                
                cpu->pc = seg_offset(segment, offset);
                pc_modified = true;
            }

            break;
        }

        case CALL: {
            uint16_t return_addr = cpu->pc + INST_SIZE;
            uint16_t return_offset = return_addr - (cpu->cs << SEG_SHIFT);
            uint16_t target_addr = inst.operand1;
            uint16_t target_offset = target_addr - (cpu->cs << SEG_SHIFT);
            uint16_t segment;

            if (cpu->flags & FLAG_USER_MODE)
                segment = cpu->us;
            else
                segment = cpu->cs;

            cpu_push(cpu, return_offset);

            cpu->pc = seg_offset(segment, target_offset);
            pc_modified = true;

            break;
        }

        case RET: {
            uint16_t offset = cpu_pop(cpu);
            uint16_t segment;

            if (cpu->flags & FLAG_USER_MODE)
                segment = cpu->us;
            else
                segment = cpu->cs;

            cpu->pc = seg_offset(segment, offset);
            pc_modified = true;

            break;
        }

        case IRET: {
            uint16_t int_num = cpu_pop(cpu);

            cpu->flags = cpu_pop(cpu);

            uint16_t offset = cpu_pop(cpu);
            uint16_t segment = cpu_pop(cpu);

            if (cpu->flags & FLAG_USER_MODE)
                cpu->us = segment;
            else
                cpu->cs = segment;
            
            cpu->pc = seg_offset(segment, offset);
            pc_modified = true;

            cpu->flags |= FLAG_INT_DONE;

            break;
        }

        case CLI: {
            cpu->flags &= ~FLAG_INT_ENABLED;

            break;
        }

        case STI: {
            cpu->flags |= FLAG_INT_ENABLED;

            break;
        }

        case INT: {
            uint16_t int_num = inst.operand1;

            cpu_push(cpu, cpu->cs);
            cpu_push(cpu, cpu->pc + INST_SIZE);
            cpu_push(cpu, cpu->flags);
            cpu_push(cpu, int_num);

            if (int_num == 8)
                return 8;

            uint32_t ivt_entry = IVT_ADDR + (int_num * 4);
            uint16_t segment = (cpu->memory[ivt_entry] << 8) | cpu->memory[ivt_entry + 1];
            uint16_t offset = (cpu->memory[ivt_entry + 2] << 8) | cpu->memory[ivt_entry + 3];

            cpu->cs = segment;
            cpu->pc = seg_offset(segment, offset);

            pc_modified = true;

            break;
        }

        case NOP: {
            break;
        }

        case HLT: {
            cpu->flags |= FLAG_HALTED;

            break;
        }

        default: {
            return 2;
        }
    }

    if (!pc_modified)
        cpu->pc += INST_SIZE;

    return 0;
}