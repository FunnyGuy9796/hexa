#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <linux/time.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "instruction_set.h"
#include "serial.h"

#define NS_PER_CYCLE (1000000000ULL / CYCLES_PER_SECOND)

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

int width = 320;
int height = 200;

uint16_t sleep_time_us;

void init_sdl() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("hexa",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                width, height, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_RGB332,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
}

void update_display(uint8_t *framebuffer) {
    SDL_UpdateTexture(texture, NULL, framebuffer, width);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void cleanup_sdl() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void cycle_delay(struct timespec *last_time) {
    struct timespec now, delta, sleep_time;

    clock_gettime(CLOCK_MONOTONIC, &now);

    delta.tv_sec = now.tv_sec - last_time->tv_sec;
    delta.tv_nsec = now.tv_nsec - last_time->tv_nsec;
    if (delta.tv_nsec < 0) {
        delta.tv_sec--;
        delta.tv_nsec += 1000000000L;
    }

    uint64_t elapsed_ns = delta.tv_sec * 1000000000ULL + delta.tv_nsec;

    if (elapsed_ns < NS_PER_CYCLE) {
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = NS_PER_CYCLE - elapsed_ns;

        nanosleep(&sleep_time, NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, last_time);
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

    size_t disk_size = 0;
    uint8_t *disk_data = read_file("disk.img", &disk_size);

    if (!disk_data) {
        fprintf(stderr, "Failed to read disk.img\n");

        return 1;
    }

    CPU cpu;

    init_cpu(&cpu);

    uint16_t bios_status = load_bios(&cpu, bios_data);

    if (bios_status == 0) {
        printf("No executable BIOS found...\n");

        return 1;
    }

    uint16_t prog_status = load_program(&cpu, disk_data, disk_size);

    if (prog_status == 0) {
        printf("No bootable program found...\n");

        return 1;
    }

    uint8_t *framebuffer = &cpu.memory[FRAMEBUFFER_ADDR];

    init_sdl();

    struct timespec last_time;
    clock_gettime(CLOCK_MONOTONIC, &last_time);

    bool running = true;

    while (running) {
        int status = step_program(&cpu);

        if (status) {
            running = false;
            
            break;
        }

        cpu.cycle_count++;

        cycle_delay(&last_time);

        if (cpu.cycle_count >= cpu.cycles_per_sleep) {
            cpu.cycle_count = 0;

            poll_serial(&cpu);

            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT)
                    running = false;
            }

            update_display(framebuffer);

            cpu_interrupt(&cpu, 0x01, parse_instruction(&cpu));
        }
    }

    cleanup_sdl();

    printf("\nCPU:\n  Clock Speed: %d MHz\n  R0: 0x%04x  R1: 0x%04x  R2: 0x%04x  R3: 0x%04x\n  R4: 0x%04x  R5: 0x%04x  R6: 0x%04x  R7: 0x%04x\n  PC: 0x%05x IP: 0x%02x    SP: 0x%04x  CS: 0x%04x\n  DS: 0x%04x  SS: 0x%04x  FLAGS: 0x%04x\n",
        CYCLES_PER_SECOND / 1000000, cpu.registers[0], cpu.registers[1], cpu.registers[2], cpu.registers[3], cpu.registers[4], cpu.registers[5], cpu.registers[6], cpu.registers[7], cpu.pc, cpu.ip, cpu.sp, cpu.cs, cpu.ds, cpu.ss, cpu.flags);
    
    return 0;
}