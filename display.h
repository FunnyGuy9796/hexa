#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

#define TM_MEMORY 0x510

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

void display_cleanup(int sig);

void display_draw(CPU *cpu);

#endif