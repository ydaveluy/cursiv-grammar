; A document of my own: one equ per adjacent pair of the expression ladder's
; ten levels (NASM manual section 3.5), tightest to loosest, plus the
; bitwise-vs-comparison pair a wrong order would get backwards.
        section .data
mulAdd          equ 2+3*4
addShift        equ 1+2<<3
shiftAnd        equ 1<<2&3
andXor          equ 1&2^3
xorOr           equ 1^2|3
orCompare       equ 1|2<3
andCompare      equ 1&2==3
compareBoolAnd  equ 1<2&&3
boolAndBoolXor  equ 1&&2^^3
boolXorBoolOr   equ 1^^2||3
