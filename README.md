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
- Partial Exceptions
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

# Run the emulator
./hexa -bios bios.bin
```
Currently, the BIOS does not support dynamic disk loading. Therefore, test.bin is required by the emulator for testing purposes.

## Contributing
Because this project and CPU design are in an early stage this is not open to contributions but advice and suggestions are strongly encouraged.