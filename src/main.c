#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "cpu.h"
#include "instruction_set.h"
#include "serial.h"

uint16_t sleep_time_us;

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

    uint8_t *buffer = (uint8_t *)malloc(file_size);

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
    if (argc <= 1) {
        printf("Options:\n  -bios | Provides the emulator with the BIOS\n  -h | Displays this list\n");

        return 0;
    }

    char *bios_name = NULL;
    char *filename = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-bios") == 0 && i + 1 < argc)
            bios_name = argv[++i];
        else if (strcmp(argv[i], "-h") == 0) {
            printf("Options:\n  -bios | Provides the emulator with the BIOS\n  -h | Displays this list\n");

            return 0;
        }
    }

    if (bios_name == NULL) {
        fprintf(stderr, "No BIOS provided\n  Use -h to see a list of all available options\n");

        return 1;
    }

    size_t bios_size = 0;
    uint8_t *bios_data = read_file(bios_name, &bios_size);

    if (!bios_data) {
        fprintf(stderr, "Failed to read BIOS file\n");

        return 1;
    }

    size_t prog_size = 0;
    uint8_t *prog_data = read_file("test.bin", &prog_size);

    if (!prog_data) {
        fprintf(stderr, "Failed to read test.bin\n");

        return 1;
    }

    CPU cpu;

    init_cpu(&cpu);

    uint16_t bios_status = load_program(&cpu, bios_data);

    if (bios_status == 0) {
        printf("No executable BIOS found...\n");

        return 1;
    }

    uint16_t prog_status = load_program(&cpu, prog_data);

    if (prog_status == 0) {
        printf("No test.bin program found...\n");

        return 1;
    }

    while (!cpu.halted) {
        sleep_time_us = (1000000ULL * cpu.cycles_per_sleep) / CYCLES_PER_SECOND;
        step_program(&cpu);
        cpu.cycle_count++;

        poll_serial(&cpu);

        if (cpu.cycle_count >= cpu.cycles_per_sleep) {
            cpu.cycle_count = 0;

            cpu_interrupt(&cpu, 0x01, parse_instruction(&cpu));
            usleep(sleep_time_us);
        }
    }

    printf("\nCPU:\n  Clock Speed: %d MHz\n  R0: 0x%04x  R1: 0x%04x  R2: 0x%04x  R3: 0x%04x\n  R4: 0x%04x  R5: 0x%04x  R6: 0x%04x  R7: 0x%04x\n  PC: 0x%05x IP: 0x%02x    SP: 0x%04x  CS: 0x%04x\n  DS: 0x%04x  SS: 0x%04x  FLAGS: 0x%04x\n",
        CYCLES_PER_SECOND / 1000000, cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3], cpu.registers[4], cpu.registers[5], cpu.registers[6], cpu.registers[7], cpu.pc, cpu.ip, cpu.sp, cpu.cs, cpu.ds, cpu.ss, cpu.flags);
    
    return 0;
}