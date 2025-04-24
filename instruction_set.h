#ifndef ISA_H
#define ISA_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "common.h"

#define MODE_VAL_IMM 0x00
#define MODE_VAL_IND 0x01

enum ISA {
    MOV = 0x00,
    LD = 0x01,
    ST = 0x02,
    PUSH = 0x03,
    POP = 0x04,
    ADD = 0x05,
    SUB = 0x06,
    INC = 0x07,
    DEC = 0x08,
    AND = 0x09,
    OR = 0x0a,
    XOR = 0x0b,
    NOT = 0x0c,
    SHL = 0x0d,
    SHR = 0x0e,
    CMP = 0x0f,
    JMP = 0x10,
    JZ = 0x11,
    JNZ = 0x13,
    JE = 0x14,
    JNE = 0x15,
    JL = 0x16,
    JLE = 0x17,
    JG = 0x18,
    JGE = 0x19,
    CALL = 0x1a,
    RET = 0x1b,
    IRET = 0x1c,
    INT = 0x1d,
    CLI = 0x1e,
    STI = 0x1f,
    NOP = 0x20,
    HLT = 0xff
};

size_t get_instruction_size(Instruction inst);
Instruction parse_instruction(CPU *cpu);
int exec_instruction(CPU *cpu, Instruction inst);

#endif