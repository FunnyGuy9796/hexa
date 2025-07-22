#include "cpu.h"
#include "instruction_set.h"

inline void cpu_push(CPU *cpu, uint16_t val) {
    uint32_t addr;

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)(val & 0xff);

    cpu->sp--;
    addr = seg_offset(cpu->ss, cpu->sp);
    cpu->memory[addr] = (uint8_t)((val >> 8) & 0xff);
}

inline uint16_t cpu_pop(CPU *cpu) {
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

inline uint32_t seg_offset(uint16_t segment, uint16_t offset) {
    return (((uint32_t)segment << SEG_SHIFT) + offset) & ADDR_MASK;
}

void init_cpu(CPU *cpu) {
    cpu->cycle_count = 0;
    cpu->cycles_per_sleep = 0;
    cpu->cs = 0xf000;
    cpu->pc = BIOS_ADDR;
    cpu->flags |= FLAG_INT_DONE;
    
    for (size_t i = 0; i < MEM_SIZE; i++)
        cpu->memory[i] = 0;
    
    cpu->memory[SERIAL_STATUS] |= SERIAL_STATUS_TX_READY;
}

uint32_t load_bios(CPU *cpu, uint8_t *bios) {
    uint32_t start_addr = (bios[0] << 16) | (bios[1] << 8) | bios[2];
    start_addr &= ADDR_MASK;

    uint32_t size = (bios[3] << 16) | (bios[4] << 8) | bios[5];
    size &= ADDR_MASK;

    if (bios[size - 2] == 0x88 && bios[size - 1] == 0xcc) {
        for (size_t i = 0; i < size - 6; i++)
            cpu->memory[(start_addr + i) & ADDR_MASK] = bios[i + 6];
        
        return size - 6;
    } else
        return 0;
}

uint32_t load_program(CPU *cpu, uint8_t *disk, size_t disk_size) {
    bool found = false;
    uint32_t total_loaded = 0;

    for (size_t pos = 0; pos + 6 < disk_size; pos++) {
        uint32_t size = (disk[pos + 3] << 16) | (disk[pos + 4] << 8) | disk[pos + 5];

        if (size < 8 || pos + size > disk_size) continue;

        if (disk[pos + size - 2] == 0x88 && disk[pos + size - 1] == 0xcc) {
            uint32_t start_addr = (disk[pos] << 16) | (disk[pos + 1] << 8) | disk[pos + 2];
            start_addr &= ADDR_MASK;

            uint32_t prog_len = size - 6;

            for (uint32_t i = 0; i < prog_len; i++)
                cpu->memory[(start_addr + i) & ADDR_MASK] = disk[pos + 6 + i];
            
            total_loaded += prog_len;
            found = true;

            break;
        }
    }
    
    return found ? total_loaded : 0;
}

int step_program(CPU *cpu, Instruction inst) {
    if (cpu->flags & FLAG_HALTED) return 0;
    
    int status = exec_instruction(cpu, inst);

    if (status != 0) {
        cpu_exception(cpu, status, inst);

        return 1;
    }

    return 0;
}

void cpu_interrupt(CPU *cpu, uint16_t status, Instruction inst) {
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
    uint16_t offset = cpu->memory[ivt_entry] | (cpu->memory[ivt_entry + 1] << 8);
    uint16_t segment = cpu->memory[ivt_entry + 2] | (cpu->memory[ivt_entry + 3] << 8);

    cpu->cs = segment;
    cpu->pc = seg_offset(segment, offset);
    cpu->flags &= ~FLAG_INT_DONE;
    cpu->flags &= ~FLAG_HALTED;
}

void cpu_exception(CPU *cpu, uint16_t status, Instruction inst) {
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
}