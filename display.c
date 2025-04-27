#include "display.h"

void display_cleanup(int sig) {
    printf("\033[?25h");
    printf("\033[H\033[2J");
    fflush(stdout);

    _exit(0);
}

void display_draw(CPU *cpu) {
    printf("\033[H");

    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            char c = cpu->memory[TM_MEMORY + y * SCREEN_WIDTH + x];

            if (c == 0)
                c = ' ';
            
            putchar(c);
        }

        putchar('\n');
    }

    fflush(stdout);
}