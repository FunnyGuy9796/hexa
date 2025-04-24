#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define REG_NUM 8
#define MEM_SIZE 65536

#define R0 0x00
#define R1 0x01
#define R2 0x02
#define R3 0x03
#define R4 0x04
#define R5 0x05
#define R6 0x06
#define R7 0x07

typedef struct {
    uint8_t opcode;
    uint8_t mode1;
    uint16_t operand1;
    uint8_t mode2;
    uint16_t operand2;
} Instruction;

typedef struct {
    uint16_t registers[REG_NUM];
    uint16_t pc;
    uint8_t ip;
    uint16_t sp;
    uint16_t flags;
    bool halted;
    uint8_t memory[MEM_SIZE];
} CPU;

#endif