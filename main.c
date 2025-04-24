#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "instruction_set.h"

uint8_t *read_file(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file %s\n", filename);

        return NULL;
    }

    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);

    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "Empty or invalid file\n");
        fclose(file);

        return NULL;
    }

    uint8_t *buffer = (uint8_t*)malloc(file_size);

    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);

        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    
    fclose(file);

    if (bytes_read != file_size) {
        fprintf(stderr, "Failed to read entire file\n");
        free(buffer);

        return NULL;
    }

    *size = bytes_read;
    
    return buffer;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);

        return 1;
    }

    size_t size = 0;
    uint8_t *data = read_file(argv[1], &size);

    if (!data) {
        fprintf(stderr, "Failed to read file\n");

        return 1;
    }

    printf("Searching for bootable program...\n");

    CPU cpu;

    init_cpu(&cpu);
    int load_status = load_program(&cpu, data, 55);

    if (load_status != 0) {
        printf("No bootable program found...\n");

        return 1;
    }

    exec_program(&cpu);

    printf("\nCPU:\n  R0: 0x%04x  R1: 0x%04x  R2: 0x%04x  R3: 0x%04x\n  R4: 0x%04x  R5: 0x%04x  R6: 0x%04x  R7: 0x%04x\n  PC: 0x%04x  IP: 0x%02x    SP: 0x%04x  FLAGS: 0x%02x\n",
        cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3], cpu.registers[4], cpu.registers[5], cpu.registers[6], cpu.registers[7], cpu.pc, cpu.ip, cpu.sp, cpu.flags);

    return 0;
}