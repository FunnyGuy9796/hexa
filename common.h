#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define REG_NUM 8
#define MEM_SIZE 65536

#define MODE_VAL_IMM 0x00
#define MODE_VAL_IND 0x01

#define START_ADDR 0x011e
#define IVT_ADDR 0x0010

#define FLAG_EQUAL (1 << 0)
#define FLAG_LESS (1 << 1)
#define FLAG_GREATER (1 << 2)
#define FLAG_ZERO (1 << 3)

#define SERIAL_DATA 0x011a
#define SERIAL_STATUS 0x011c
#define SERIAL_CTRL 0x011d

#define SERIAL_STATUS_TX_READY (1 << 0)
#define SERIAL_STATUS_RX_READY (1 << 1)
#define SERIAL_STATUS_OVERRUN (1 << 2)
#define SERIAL_STATUS_NEW_DATA (1 << 3)

enum REGS {
    R0 = 0x00,
    R1 = 0x01,
    R2 = 0x02,
    R3 = 0x03,
    R4 = 0x04,
    R5 = 0x05,
    R6 = 0x06,
    R7 = 0x07,
    PC = 0x08,
    IP = 0x09,
    SP = 0x0a,
    FLAGS = 0x0b
};

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
    uint16_t cycle_count;
    uint16_t cycles_per_sleep;
    bool halted;
    bool interrupts_enabled;
    bool tx_pending;
    uint8_t memory[MEM_SIZE];
} CPU;

void cpu_push(CPU *cpu, uint16_t val);
uint16_t cpu_pop(CPU *cpu);

#endif