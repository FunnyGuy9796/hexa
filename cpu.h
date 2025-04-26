#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "common.h"

#define CYCLES_PER_SECOND 6000000  // 6 MHz
#define CYCLE_TIME_US (1000000 / CYCLES_PER_SECOND)

void init_cpu(CPU *cpu);
uint16_t load_program(CPU *cpu, uint8_t *program);
int exec_program(CPU *cpu);
void cpu_exception(CPU *cpu, int status, Instruction inst);

#endif