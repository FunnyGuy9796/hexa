#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdio.h>
#include "common.h"

void write_disk(CPU *cpu);

void read_disk(CPU *cpu);

extern char *disk_name;

#endif