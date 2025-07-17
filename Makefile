CC := gcc
CFLAGS := -g -O0

SRC_DIR := src
BUILD_DIR := build

BINARIES := hexa hexa_asm

hexa_SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/cpu.c $(SRC_DIR)/instruction_set.c $(SRC_DIR)/serial.c
hexa_asm_SRCS := $(SRC_DIR)/assembler.c $(SRC_DIR)/cpu.c $(SRC_DIR)/instruction_set.c

hexa_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(hexa_SRCS))
hexa_asm_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(hexa_asm_SRCS))

all: $(BINARIES)

hexa: $(hexa_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

hexa_asm: $(hexa_asm_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BINARIES)

.PHONY: all clean