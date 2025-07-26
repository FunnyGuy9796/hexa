#ifndef CPU_H
#define CPU_H

#include "common.h"

#define CYCLES_PER_SECOND 10000000  // 10 MHz

void init_cpu(CPU *cpu);
uint32_t load_bios(CPU *cpu, uint8_t *bios);
int step_program(CPU *cpu, Instruction inst);

#endif