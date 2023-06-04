#include "localisa.asm"

start:

    nop
    nop
    movi r0 i 0xff
    movi r1 i 0xff
    add r2 r1 r0

    movi r1 i start
    jalr r0 r1
