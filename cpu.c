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

uint16_t load_program(CPU *cpu, uint8_t *program) {
    uint16_t start_addr = (program[0] << 8) | program[1];
    uint16_t size = (program[2] << 8) | program[3];

    if (program[size - 1] == 0xcc && program[size - 2] == 0x88) {
        printf("\n");
        
        for (size_t i = 0; i < size - 4; i++) {
            cpu->memory[start_addr + i] = program[i + 4];

            printf("%02x ", cpu->memory[start_addr + i]);

            if ((i + 1) % 7 == 0)
                printf("\n");
        }

        printf("\n\n");
        
        return size - 4;
    } else
        return 0;
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

        usleep(CYCLE_TIME_US);
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

    printf("\nError: Exception occurred\n  status: %d\n  inst_opcode: 0x%02x\n  inst_mode1: 0x%02x  inst_op1: 0x%04x\n  inst_mode2: 0x%02x  inst_op2: 0x%04x\n",
        status, inst.opcode, inst.mode1, inst.operand1, inst.mode2, inst.operand2);
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