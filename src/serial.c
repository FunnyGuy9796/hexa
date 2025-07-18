#include "serial.h"

void poll_serial(CPU *cpu) {
    if (cpu->memory[SERIAL_STATUS] & SERIAL_STATUS_NEW_DATA) {
        if (cpu->memory[SERIAL_STATUS] & SERIAL_STATUS_TX_READY) {
            cpu->memory[SERIAL_STATUS] &= ~SERIAL_STATUS_NEW_DATA;
            cpu->memory[SERIAL_STATUS] &= ~SERIAL_STATUS_TX_READY;
            
            putchar(cpu->memory[SERIAL_DATA]);
            fflush(stdout);

            cpu->memory[SERIAL_STATUS] |= SERIAL_STATUS_TX_READY;
        }
    }
}