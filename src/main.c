#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "instruction_set.h"
#include "serial.h"
#include "disk.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

CPU cpu;
atomic_bool running = true;
pthread_t emu_thread;

void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("hexa",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_RGB332,
                                SDL_TEXTUREACCESS_STREAMING,
                                FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
}

void update_display(uint8_t *framebuffer) {
    SDL_UpdateTexture(texture, NULL, framebuffer, FRAMEBUFFER_WIDTH);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    framebuffer_dirty = false;
}

void cleanup_sdl() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

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

void* emulator_loop(void *arg) {
    uint64_t last_ticks = SDL_GetPerformanceCounter();
    uint64_t perf_freq = SDL_GetPerformanceFrequency();

    while (running) {
        uint64_t now_ticks = SDL_GetPerformanceCounter();
        uint64_t elapsed_ticks = now_ticks - last_ticks;

        last_ticks = now_ticks;

        double elapsed_seconds = (double)elapsed_ticks / perf_freq;
        uint64_t cycles_to_run = (uint64_t)(elapsed_seconds * CYCLES_PER_SECOND);
        const uint64_t MAX_CYCLES = 100000;

        if (cycles_to_run > MAX_CYCLES)
            cycles_to_run = MAX_CYCLES;

        for (uint64_t i = 0; i < cycles_to_run && running; i++) {
            Instruction inst = parse_instruction(&cpu);
            int status = step_program(&cpu, inst);

            if (status) {
                running = false;

                break;
            }

            cpu.cycle_count++;

            if (cpu.cycle_count >= cpu.cycles_per_sleep) {
                cpu.cycle_count = 0;

                poll_serial(&cpu);
                cpu_interrupt(&cpu, 0x01);
            }
        }

        SDL_Delay(1);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Options:\n  -disk | Provides the emulator with a bootable disk\n  -h | Displays this list\n");

        return 0;
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-disk") == 0 && i + 1 < argc)
            disk_name = argv[++i];
        else if (strcmp(argv[i], "-h") == 0) {
            printf("Options:\n  -disk | Provides the emulator with a bootable disk\n  -h | Displays this list\n");

            return 0;
        }
    }

    if (disk_name == NULL) {
        fprintf(stderr, "No disk provided\n  Use -h to see a list of all available options\n");

        return 1;
    }

    size_t bios_size = 0;
    uint8_t *bios_data = read_file("bios.bin", &bios_size);

    if (!bios_data) {
        fprintf(stderr, "Failed to read BIOS file\n");

        return 1;
    }

    size_t disk_size = 0;
    uint8_t *disk_data = read_file(disk_name, &disk_size);

    if (!disk_data) {
        fprintf(stderr, "Failed to read disk.img\n");

        return 1;
    }

    init_cpu(&cpu);

    uint16_t bios_status = load_bios(&cpu, bios_data);

    if (bios_status == 0) {
        printf("No executable BIOS found...\n");

        return 1;
    }

    init_sdl();
    update_display(&cpu.memory[FRAMEBUFFER_ADDR]);

    pthread_create(&emu_thread, NULL, emulator_loop, NULL);

    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        if (framebuffer_dirty)
            update_display(&cpu.memory[FRAMEBUFFER_ADDR]);

        SDL_Delay(16);
    }

    pthread_join(emu_thread, NULL);
    cleanup_sdl();

    printf("\nCPU:\n  Clock Speed: %d MHz\n  R0: 0x%04x  R1: 0x%04x  R2: 0x%04x  R3: 0x%04x\n  R4: 0x%04x  R5: 0x%04x  R6: 0x%04x  R7: 0x%04x\n  PC: 0x%05x IP: 0x%02x    SP: 0x%04x  CS: 0x%04x\n  DS: 0x%04x  SS: 0x%04x  US: 0x%04x  FLAGS: 0x%04x\n",
        CYCLES_PER_SECOND / 1000000, cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3], cpu.registers[4], cpu.registers[5], cpu.registers[6], cpu.registers[7], cpu.pc, cpu.ip, cpu.sp, cpu.cs, cpu.ds, cpu.ss, cpu.us, cpu.flags);
    
    return 0;
}