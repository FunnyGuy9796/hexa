#include "disk.h"
#include "instruction_set.h"

char *disk_name = NULL;

void write_disk(CPU *cpu) {
    if (disk_name == NULL) {
        printf("disk_name == NULL\n");

        return;
    }

    FILE *disk = fopen(disk_name, "wb");

    if (disk == NULL) {
        printf("disk == NULL\n");

        fclose(disk);

        return;
    }

    uint8_t *status = &cpu->memory[DISK_STATUS];
    uint16_t lba = (cpu->memory[DISK_LBA] << 8) | cpu->memory[DISK_LBA + 1];
    uint8_t count = (cpu->memory[DISK_COUNT] << 8) | cpu->memory[DISK_COUNT + 1];
    uint16_t segment = cpu->registers[R6];
    uint16_t offset = cpu->registers[R7];
    uint32_t phys_addr = seg_offset(segment, offset);

    if (!(*status & DISK_STATUS_READY) || (*status & DISK_STATUS_BUSY))
        return;
    
    *status &= ~DISK_STATUS_READY;
    *status |= DISK_STATUS_BUSY;

    fseek(disk, lba * 512, SEEK_SET);

    size_t written = fwrite(&cpu->memory[phys_addr], 512, count, disk);

    if (written == count) {
        *status |= DISK_STATUS_READY;
        *status |= DISK_STATUS_DONE;
        *status &= ~DISK_STATUS_BUSY;
    } else {
        *status |= DISK_STATUS_ERROR;

        printf("error writing to disk\n");
    }
}

void read_disk(CPU *cpu) {
    if (disk_name == NULL) {
        printf("disk_name == NULL\n");

        return;
    }
    
    FILE *disk = fopen(disk_name, "rb");

    if (disk == NULL) {
        printf("disk == NULL\n");

        fclose(disk);

        return;
    }

    uint8_t *status = &cpu->memory[DISK_STATUS];
    uint16_t lba = (cpu->memory[DISK_LBA] << 8) | cpu->memory[DISK_LBA + 1];
    uint8_t count = (cpu->memory[DISK_COUNT] << 8) | cpu->memory[DISK_COUNT + 1];
    uint16_t segment = cpu->registers[R6];
    uint16_t offset = cpu->registers[R7];
    uint32_t phys_addr = seg_offset(segment, offset);

    if (!(*status & DISK_STATUS_READY) || (*status & DISK_STATUS_BUSY))
        return;

    *status &= ~DISK_STATUS_READY;
    *status |= DISK_STATUS_BUSY;

    fseek(disk, lba * 512, SEEK_SET);

    size_t read = fread(&cpu->memory[phys_addr], 512, count, disk);

    if (read == count) {
        *status |= DISK_STATUS_READY;
        *status |= DISK_STATUS_DONE;
        *status &= ~DISK_STATUS_BUSY;

        if (phys_addr >= FRAMEBUFFER_ADDR && phys_addr <= FRAMEBUFFER_ADDR + (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT))
            framebuffer_dirty = true;

        cpu->memory[DISK_COMMAND] = 0x0000;
    } else {
        *status |= DISK_STATUS_ERROR;

        printf("error reading from disk\n");
    }
}