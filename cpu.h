#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "common.h"

typedef struct {
    bool zero;
    bool carry;
    bool negative;
    bool overflow;
} Flags;

void init_cpu(CPU *cpu);
int load_program(CPU *cpu, uint8_t *program, size_t size);
int exec_program(CPU *cpu);
void cpu_exception(CPU *cpu, int status, Instruction inst);

#endif