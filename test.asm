org 0x0d1c

start:
    mov SP, #0xffff
    mov R0, #10
    mov R1, #0
    cmp R0, R1
    jg test
    hlt

test:
    mov R5, #3
    hlt

db 0x88
db 0xcc