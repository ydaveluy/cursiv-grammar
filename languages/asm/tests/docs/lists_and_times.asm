; A document of my own: `dup`/`%(...)` initializer lists, `times`, `incbin`,
; a question-mark placeholder, and a segment-qualified register operand --
; constructs the reference grammar accepts but this port's upstream corpus
; never exercises.
        section .data
buf:    db 10 dup(0)
lst:    dw %(1, 2, 3, 4)
sized:  dd word %(5, 6)
pad:    times 5 db 0
hole:   db ?
LEN     equ 5 >= 3

        section .text
        incbin "payload.bin"
        mov ax, es:bx
        mov cx, [es:buf]
        ret
