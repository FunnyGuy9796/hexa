# Hexa
Hexa is a custom 16-bit CPU architecture that aims to be efficient and versatile.

## Features
- 20-bit Address Bus
- 8 General Purpose Registers
- 3 Segment Selector Registers
- 10 MHz Clock Speed (variable speed)
- Interrupts
  - Timer
  - Keyboard
  - Serial (BIOS Software Interrupt)
- Exceptions
- Simple Assembler

## Usage
``` bash
# Clone the project
git clone https://github.com/FunnyGuy9796/hexa.git

# Compile the project
make all

# Assemble BIOS and test.asm
./hexa_asm -f bios.asm -o bios.bin
./hexa_asm -f test.asm -o test.bin
cat test.bin > disk.img

# Run the emulator
./hexa -disk disk.img
```
Currently, the BIOS does not support dynamic disk loading. Therefore, test.bin is required by the emulator for testing purposes.

## Contributing
Because this project and CPU design are in an early stage this is not open to contributions but advice and suggestions are strongly encouraged.

## Documentation
Programming using the Hexa ISA is fairly simple as it is designed to somewhat resemble x86 assembly.

### Instruction Format
All instructions are 8 bytes in size including a byte for padding in order to maintain memory alignment.

An instruction that may look like `mov R0, #1` will be assembled to the bytes `0x00 0x01 0x00 0x00 0x00 0x00 0x01 0x00`.
- The first byte `0x00` corresponds to the opcode (the `mov` instruction)
- The second byte `0x01` corresponds to the mode of the first operand (`0x00` for immediate and `0x01` for indirect)
- The third and fourth bytes corresponds to the first operand (`0x00 0x00` for R0)
- The fifth byte serves the same purpose as the second byte, indicating the mode of the second operand
- The sixth and seventh bytes `0x00 0x01` correspond to the second operand (the literal `#1`)
- Number literals must be denoted by `#` in order for the assembler to recognize them properly
  - This includes any usage of memory addresses as the address itself is treated as a literal number by the assembler and is only identified as an address by the CPU
- All general and special registers may be referenced simply by their name and any usage will always refer to their immediate value
- All instructions (with the exception of `ST` and `LD`) only allow interaction between registers and immediate values
  - Any incorrect usage of values in an instruction will generate an "Invalid Operand" exception (see [CPU Exceptions](#cpu-exceptions))
- All memory access done by programs is required to be word-aligned and any unaligned access will generate an "Unaligned Access" exception (see [CPU Exceptions](#cpu-exceptions))

### CPU Interrupts
| Number | Description |
|--------|-------------|
| `0x01` | Timer       |
| `0x02` | Keyboard    |
| `0x03` | Serial Port |
| `0x04` | Disk        |

### CPU Exceptions
| Number | Description                |
|--------|----------------------------|
| `0x01` | Invalid Opcode             |
| `0x02` | Invalid Operand            |
| `0x03` | Unaligned Access           |
| `0x04` | General Protection         |
| `0x05` | Illegal Instruction        |
| `0x06` | Divide by Zero             |
| `0x07` | Stack Overflow / Underflow |
| `0x08` | Breakpoint                 |

### Memory Map
| Type                      | Start     | End       | Size      |
|---------------------------|-----------|-----------|-----------|
| General Purpose Registers | `0x00000` | `0x00007` | 8 bytes   |
| Special Registers         | `0x00008` | `0x0000f` | 8 bytes   |
| Interupt Vector Table     | `0x00010` | `0x0010f` | 256 bytes |
| Keyboard                  | `0x00110` | `0x00115` | 6 bytes   |
| Timer                     | `0x00116` | `0x0011f` | 10 bytes  |
| Serial Port               | `0x00120` | `0x00125` | 6 bytes   |
| Disk                      | `0x00126` | `0x0012f` | 10 bytes  |
| Usable Memory             | `0x00130` | `0xdffff` | 917.24 KB |
| Framebuffer               | `0xe0000` | `0xeffff` | 64 KB     |
| Usable Memory             | `0xf0000` | `0xffe67` | 65.18 KB  |
| BIOS                      | `0xffe68` | `0xfffff` | 416 bytes |