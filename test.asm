org 0x0d17

start:
    mov R0, #2
    mov R1, #5
    mov R2, R1
    st 0xff00, R0
    ld R6, 0xff00
    mov SP, #0xffff
    add R1, #2
    inc R0
    cmp R0, R1
    hlt

db 0x88
db 0xcc