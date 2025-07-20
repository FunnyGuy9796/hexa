org 0x1000:0x011e

start:
    mov SS, #0x000f
    mov SP, #0xffff

    mov DS, #0x0000

    mov R7, #0x0a
    int #0x03

    mov R7, #0x48
    int #0x03

    mov R7, #0x65
    int #0x03

    mov R7, #0x6c
    int #0x03

    mov R7, #0x6c
    int #0x03

    mov R7, #0x6f
    int #0x03

    mov R7, #0x0a
    int #0x03

    mov R0, #0x011e

    jmp draw

draw:
    st R0, #0xefef

    add R0, #2
    cmp R0, #0xfffe
    je inc_ds

    mov R1, DS
    cmp R1, #0x1000
    je check_mem

    jmp draw

inc_ds:
    mov DS, #0x1000
    jmp draw

check_mem:
    mov R7, #0x62
    int #0x03

    mov R7, #0x79
    int #0x03

    mov R7, #0x65
    int #0x03

    mov R7, #0x0a
    int #0x03

    hlt

dw 0x88cc