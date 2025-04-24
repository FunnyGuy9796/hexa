#include <stdio.h>
#include "cpu.h"
#include "instruction_set.h"

int main() {
    uint8_t program[] = {
        MOV, MODE_VAL_IND, R0, 0x00, MODE_VAL_IMM, 0x03, 0x00,
        MOV, MODE_VAL_IND, R1, 0x00, MODE_VAL_IND, R0, 0x00,
        MOV, MODE_VAL_IND, R7, 0x00, MODE_VAL_IMM, 0x41, 0x00,
        HLT,
        0x88, 0xcc
    };

    CPU cpu;

    init_cpu(&cpu);
    int load_status = load_program(&cpu, program, 24);

    if (load_status != 0) {
        printf("No bootable program found...\n");

        return 1;
    }

    exec_program(&cpu);

    printf("\nCPU:\n  R0: 0x%04x  R1: 0x%04x  R2: 0x%04x  R3: 0x%04x\n  R4: 0x%04x  R5: 0x%04x  R6: 0x%04x  R7: 0x%04x\n  PC: 0x%04x  IP: 0x%02x    SP: 0x%04x  FLAGS: 0x%02x\n",
        cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3], cpu.registers[4], cpu.registers[5], cpu.registers[6], cpu.registers[7], cpu.pc, cpu.ip, cpu.sp, cpu.flags);

    return 0;
}