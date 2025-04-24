#include "cpu.h"
#include "instruction_set.h"

void init_cpu(CPU *cpu) {
    for (size_t i = 0; i < REG_NUM; i++)
        cpu->registers[i] = 0;
    
    cpu->pc = 0;
    cpu->sp = 0;
    cpu->flags = 0;
    cpu->halted = false;
    
    for (size_t i = 0; i < MEM_SIZE; i++)
        cpu->memory[i] = 0;
}

int load_program(CPU *cpu, uint8_t *program, size_t size) {
    uint16_t start_addr = (program[0] << 8) | program[1];

    if (program[size - 1] == 0xcc && program[size - 2] == 0x88) {
        printf("\n");
        
        for (size_t i = 0; i < size - 2; i++) {
            cpu->memory[start_addr + i] = program[i + 2];

            printf("%02x ", cpu->memory[start_addr + i]);

            if ((i + 1) % 7 == 0)
                printf("\n");
        }

        printf("\n\n");
        
        return 0;
    } else
        return 1;
}

int exec_program(CPU *cpu) {
    cpu->pc = START_ADDR;
    cpu->ip = cpu->memory[cpu->pc];
    
    while (!cpu->halted) {
        Instruction inst = parse_instruction(cpu);
        int status = exec_instruction(cpu, inst);

        if (status != 0) {
            cpu_exception(cpu, status, inst);

            return 1;
        }
    }

    return 0;
}

void cpu_exception(CPU *cpu, int status, Instruction inst) {
    size_t inst_size = get_instruction_size(inst);

    cpu->halted = true;

    cpu_push(cpu, cpu->pc + inst_size);
    cpu_push(cpu, cpu->pc);
    cpu_push(cpu, cpu->ip);
    cpu_push(cpu, cpu->flags);

    printf("Error: Exception occurred\n  status: %d\n", status);
}

void cpu_push(CPU *cpu, uint16_t val) {
    cpu->memory[--cpu->sp] = (uint8_t)(val & 0xff);
    cpu->memory[--cpu->sp] = (uint8_t)((val >> 8) & 0xff);
}

uint16_t cpu_pop(CPU *cpu) {
    uint8_t hi = cpu->memory[cpu->sp++];
    uint8_t lo = cpu->memory[cpu->sp++];

    return (hi << 8) | lo;
}