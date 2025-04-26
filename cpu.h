#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "common.h"

void init_cpu(CPU *cpu);
uint16_t load_program(CPU *cpu, uint8_t *program);
int exec_program(CPU *cpu);
void cpu_exception(CPU *cpu, int status, Instruction inst);

#endif