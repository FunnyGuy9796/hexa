org 0x051e

start:
    mov SP, #0xffff
    mov R0, #0x051a

    st R0, #0x48
    ld R1, R0

    st R0, #0x65
    ld R1, R0

    st R0, #0x6c
    ld R1, R0

    st R0, #0x6c
    ld R1, R0

    st R0, #0x6f
    ld R1, R0

    hlt

db 0x88
db 0xcc