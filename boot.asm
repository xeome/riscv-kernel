;;; Initial boot sector

testing:
    jmp testing             ; Jump to label continuously

    times 510-($-$$) db 0   ; Write 0 until 510th byte

    dw 0xaa55               ; BIOS magic number