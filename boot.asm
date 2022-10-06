;;; Initial boot sector

    org 0x7c00                  ; Origin

    mov ah, 0x0e                ; Set ah to 0x0e so int 10 outputs to teletype
    mov bx, string1             ; Move address of string1 to bx
    
    call printstr
    
    jmp halt_program

printstr:
        mov al, [bx]            ; Move character value at address in bx into al
        cmp al, 0               ; Check for null
        je end_print            ; Jump to end_print if previous condition is true, continues if condition isn't true
        int 0x10                ; Print character on register al
        add bx, 1               ; Increment address of bx to get to the next character in string
        jmp printstr            ; Loop

end_print:
        ret

string1:    db 'Hello World!', 0   ; 0 is for null termination

halt_program:
        jmp $                   ; continuous jump
        
        times 510-($-$$) db 0   ; Fill with 0 until 510th byte

        dw 0xaa55               ; BIOS magic number to set bootable, 0xaa55 on little endian, 0x55aa on big endian