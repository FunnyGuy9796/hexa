#include "cpu.h"
#include "instruction_set.h"

inline void cpu_push(CPU *cpu, uint16_t val) {
    uint32_t addr;
    uint32_t curr_addr = seg_offset(cpu->ss, cpu->sp);

    if (curr_addr >= BIOS_ADDR || curr_addr < START_ADDR)
        cpu_exception(cpu, 0x07);

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)((val >> 8) & 0xff);

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)(val & 0xff);
}

inline uint16_t cpu_pop(CPU *cpu) {
    uint32_t addr;
    uint8_t lo, hi;
    uint32_t curr_addr = seg_offset(cpu->ss, cpu->sp);

    if (curr_addr >= BIOS_ADDR || curr_addr < START_ADDR)
        cpu_exception(cpu, 0x07);

    addr = seg_offset(cpu->ss, cpu->sp);
    lo = cpu->memory[addr];
    cpu->sp++;

    addr = seg_offset(cpu->ss, cpu->sp);
    hi = cpu->memory[addr];
    cpu->sp++;

    return (hi << 8) | lo;
}

inline uint32_t seg_offset(uint16_t segment, uint16_t offset) {
    return (((uint32_t)segment << SEG_SHIFT) + offset) & ADDR_MASK;
}

void init_cpu(CPU *cpu) {
    cpu->cycle_count = 0;
    cpu->cycles_per_sleep = 0;
    cpu->cs = 0x0000;
    cpu->pc = 0x0000;
    cpu->flags |= (FLAG_INT_DONE | FLAG_RESET);
    
    for (size_t i = 0; i < MEM_SIZE; i++)
        cpu->memory[i] = 0;
    
    cpu->memory[0x00000] = 0x00;
    cpu->memory[0x00001] = 0x01;
    cpu->memory[0x00002] = 0x00;
    cpu->memory[0x00003] = 0x08;
    cpu->memory[0x00004] = 0x00;
    cpu->memory[0x00005] = 0xf0;
    cpu->memory[0x00006] = 0x00;
    cpu->memory[0x00007] = 0x00;
    cpu->memory[0x00008] = 0x10;
    cpu->memory[0x00009] = 0x00;
    cpu->memory[0x0000a] = 0xfb;
    cpu->memory[0x0000b] = 0xe6;
    cpu->memory[0x0000c] = 0x01;
    cpu->memory[0x0000d] = 0x00;
    cpu->memory[0x0000e] = 0x00;
    cpu->memory[0x0000f] = 0x00;
    
    cpu->memory[SERIAL_STATUS] |= SERIAL_STATUS_TX_READY;
    cpu->memory[DISK_STATUS] |= DISK_STATUS_READY;
}

uint32_t load_bios(CPU *cpu, uint8_t *bios) {
    uint32_t start_addr = (bios[2] << 16) | (bios[3] << 8) | bios[4];
    start_addr &= ADDR_MASK;

    uint32_t size = (bios[6] << 16) | (bios[7] << 8) | bios[8];
    size &= ADDR_MASK;

    if (bios[0] == 0x88 && bios[1] == 0xcc) {
        for (size_t i = 10; i < size; i++)
            cpu->memory[(start_addr + (i - 10)) & ADDR_MASK] = bios[i];
        
        return size - 10;
    } else
        return 0;
}

int step_program(CPU *cpu, Instruction inst) {
    if (cpu->flags & FLAG_HALTED) return 0;
    
    int status = exec_instruction(cpu, inst);

    if (status != 0) {
        cpu_exception(cpu, status);

        return 1;
    }

    return 0;
}

void cpu_interrupt(CPU *cpu, uint16_t status) {
    if (!(cpu->flags & FLAG_INT_ENABLED) || !(cpu->flags & FLAG_INT_DONE))
        return;

    if (cpu->flags & FLAG_USER_MODE)
        cpu_push(cpu, cpu->us);
    else
        cpu_push(cpu, cpu->cs);
    
    cpu_push(cpu, cpu->pc + INST_SIZE);
    cpu_push(cpu, cpu->flags);
    cpu_push(cpu, status);

    cpu->flags &= ~FLAG_USER_MODE;

    uint32_t ivt_entry = IVT_ADDR + (status * 4);
    uint16_t segment = (cpu->memory[ivt_entry] << 8) | cpu->memory[ivt_entry + 1];
    uint16_t offset = (cpu->memory[ivt_entry + 2] << 8) | cpu->memory[ivt_entry + 3];

    cpu->cs = segment;
    cpu->pc = seg_offset(segment, offset);
    cpu->flags &= ~FLAG_INT_DONE;
    cpu->flags &= ~FLAG_HALTED;
}

void cpu_exception(CPU *cpu, uint16_t status) {
    if (!(cpu->flags & FLAG_EXCEPTION)) {
        cpu->flags |= FLAG_EXCEPTION;
        cpu->flags &= ~FLAG_INT_ENABLED;
        cpu->flags |= FLAG_HALTED;

        cpu_push(cpu, cpu->pc + INST_SIZE);
        cpu_push(cpu, cpu->pc);
        cpu_push(cpu, cpu->ip);
        cpu_push(cpu, cpu->flags);
        cpu_push(cpu, status);

        printf("\nError: Exception occurred\n  addr: 0x%05x\n  status: %d\n  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            cpu->pc, status, cpu->memory[cpu->pc], cpu->memory[cpu->pc + 1], cpu->memory[cpu->pc + 2], cpu->memory[cpu->pc + 3],
            cpu->memory[cpu->pc + 4], cpu->memory[cpu->pc + 5], cpu->memory[cpu->pc + 6], cpu->memory[cpu->pc + 7]);
        
        printf("\n");
    } else {
        cpu->flags |= FLAG_DOUBLE_EXCEPTION;

        printf("\nError: Double exception occurred\n");
    }
}