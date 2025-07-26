CC := gcc
CFLAGS := -g -O0 $(shell sdl2-config --cflags)
LDFLAGS := $(shell sdl2-config --libs)

SRC_DIR := src
BUILD_DIR := build

BINARIES := hexa hexa_asm

hexa_SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/cpu.c $(SRC_DIR)/instruction_set.c $(SRC_DIR)/serial.c $(SRC_DIR)/disk.c
hexa_asm_SRCS := $(SRC_DIR)/assembler.c $(SRC_DIR)/cpu.c $(SRC_DIR)/instruction_set.c $(SRC_DIR)/disk.c

hexa_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(hexa_SRCS))
hexa_asm_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(hexa_asm_SRCS))

all: $(BINARIES)

hexa: $(hexa_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

hexa_asm: $(hexa_asm_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BINARIES)

.PHONY: all clean