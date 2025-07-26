#include "serial.h"

void poll_serial(CPU *cpu) {
    uint8_t *status = &cpu->memory[SERIAL_STATUS];
    uint8_t data = cpu->memory[SERIAL_DATA];
    
    if (!((*status & SERIAL_STATUS_NEW_DATA) && (*status & SERIAL_STATUS_TX_READY)))
        return;

    *status &= ~SERIAL_STATUS_TX_READY;

    putchar(data);
    fflush(stdout);

    *status &= ~SERIAL_STATUS_NEW_DATA;
    *status |= SERIAL_STATUS_TX_READY;
}