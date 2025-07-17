#ifndef CPU_H
#define CPU_H

#include "common.h"

#define CYCLES_PER_SECOND 10000000  // 10 MHz

void init_cpu(CPU *cpu);
uint32_t load_program(CPU *cpu, uint8_t *program);
int step_program(CPU *cpu);
void cpu_interrupt(CPU *cpu, uint16_t status, Instruction inst);
void cpu_exception(CPU *cpu, uint16_t status, Instruction inst);

#endif