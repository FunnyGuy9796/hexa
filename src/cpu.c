#include "cpu.h"
#include "instruction_set.h"

void cpu_push(CPU *cpu, uint16_t val) {
    uint32_t addr;

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)(val & 0xff);

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)((val >> 8) & 0xff);
}

uint16_t cpu_pop(CPU *cpu) {
    uint32_t addr;
    uint8_t lo, hi;

    addr = seg_offset(cpu->ss, cpu->sp);
    hi = cpu->memory[addr];
    cpu->sp++;

    addr = seg_offset(cpu->ss, cpu->sp);
    lo = cpu->memory[addr];
    cpu->sp++;

    return (hi << 8) | lo;
}

uint32_t seg_offset(uint16_t segment, uint16_t offset) {
    return (((uint32_t)segment << SEG_SHIFT) + offset) & ADDR_MASK;
}

void init_cpu(CPU *cpu) {
    cpu->cycle_count = 0;
    cpu->cycles_per_sleep = 0;
    cpu->halted = false;
    cpu->interrupts_enabled = false;
    cpu->cs = 0xffcf;
    cpu->pc = 0xffcff;
    
    for (size_t i = 0; i < MEM_SIZE; i++)
        cpu->memory[i] = 0;
}

uint32_t load_program(CPU *cpu, uint8_t *program) {
    uint32_t start_addr = (program[0] << 16) | (program[1] << 8) | program[2];
    start_addr &= ADDR_MASK;

    uint32_t size = (program[3] << 16) | (program[4] << 8) | program[5];
    size &= ADDR_MASK;

    if (program[size - 1] == 0xcc && program[size - 2] == 0x88) {
        for (size_t i = 0; i < size - 6; i++)
            cpu->memory[(start_addr + i) & ADDR_MASK] = program[i + 6];
        
        return size - 6;
    } else
        return 0;
}

int step_program(CPU *cpu) {
    Instruction inst = parse_instruction(cpu);
    int status = exec_instruction(cpu, inst);

    if (status != 0) {
        cpu_exception(cpu, status, inst);

        return 1;
    }

    return 0;
}

void cpu_interrupt(CPU *cpu, uint16_t status, Instruction inst) {
    if (!cpu->interrupts_enabled)
        return;

    cpu_push(cpu, cpu->cs);
    cpu_push(cpu, cpu->pc + INST_SIZE);
    cpu_push(cpu, cpu->flags);
    cpu_push(cpu, status);

    uint32_t ivt_entry = IVT_ADDR + (status * 4);
    uint16_t offset = cpu->memory[ivt_entry] | (cpu->memory[ivt_entry + 1] << 8);
    uint16_t segment = cpu->memory[ivt_entry + 2] | (cpu->memory[ivt_entry + 3] << 8);

    cpu->cs = segment;
    cpu->pc = seg_offset(segment, offset);
}

void cpu_exception(CPU *cpu, uint16_t status, Instruction inst) {
    cpu->halted = true;

    cpu_push(cpu, cpu->pc + INST_SIZE);
    cpu_push(cpu, cpu->pc);
    cpu_push(cpu, cpu->ip);
    cpu_push(cpu, cpu->flags);
    cpu_push(cpu, status);

    printf("\nError: Exception occurred\n  addr: 0x%05x\n  status: %d\n  inst_opcode: 0x%02x\n  inst_mode1: 0x%02x  inst_op1: 0x%04x\n  inst_mode2: 0x%02x  inst_op2: 0x%04x\n",
        cpu->pc, status, inst.opcode, inst.mode1, inst.operand1, inst.mode2, inst.operand2);
}