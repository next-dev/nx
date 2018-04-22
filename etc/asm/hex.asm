; Hex editor
; Based on code from Your Sinclair Program Pitstop (issue 46)

        org   25000
        opt   Start:Start

        load  "rom.asm"

editline      equ 11

Start:
        ; Clear screen
        ld    hl,$4000          ; Start of pixel data
        ld    de,$4001
        xor   a
        ld    (hl),a
        ld    bc,$1800
        ldir                    ; Clear pixels

        ; Title bar        
        ld    bc,32
        ld    (hl),%01000100    ; Green ink, black paper, bright
        ldir

        ; Column headers
        ld    bc,32
        ld    (hl),%00011111    ; White ink, magenta paper, bright
        ldir

        ; Background
        ld    bc,22*32-1
        ld    (hl),%00100000    ; Black ink, green paper
        ldir

        ; Edit line
        ld    hl,$5800+(editline*32)
        ld    de,$5800+(editline*32)+1
        ld    (hl),%01100000    ; Black ink, green paper, bright
        ld    bc,31
        ldir

        ; Output the string
        ld    de,titleString
        ld    hl,256*(16-(10/2))+0
        call  printString

        jp    $

titleString:
        db    "Hex Loader\0"

printString:
        ld    a,(de)            ; Get character
        inc   de
        cp    0                 ; Terminal character?
        ret   z                 ; Yes, return
        push  de                ; Save string pointer
        ld    d,a
        call  printChar         ; Print character
        pop   de
        inc   h                 ; Increase X position
        jr    PrintString       ; Next character

printChar:
        ; H = X position
        ; L = Y position
        ; D = character
        push  hl                ; Save position
        push  bc
        push  de

        ; Calculate screen address
        ; 010SSRRR CCCXXXXX
        ld    a,$40             ; VRAM position
        or    l                 ; Place Y-pos in S part
        and   %11111000         ; Zero out RRR
        ld    b,a               ; B = high part
        ld    a,l               ; A = Y-pos (000CCCCC)
        rrca                    ; C000CCCC
        rrca                    ; CC000CCC
        rrca                    ; CCC000CC
        and   %11100000         ; CCC00000
        or    h                 ; CCCXXXXX
        ld    l,a
        ld    h,b               ; HL = screen address
        push  hl
        ld    l,d
        ld    h,0               ; ASCII character code (HL)
        add   hl,hl             ; *2
        add   hl,hl             ; *4
        add   hl,hl             ; *8
        ex    de,hl             ; DE = font offset
        ld    hl,(CHARS)        ; Get the address of the current font
        add   hl,de             ; Get font address of character
        ex    de,hl             ; DE = font address
        pop   hl                ; HL = screen address

        ; Print the character to the screen in bold
        ld    b,8
.l1     ld    a,(de)            ; A = pixel data for current line
        ld    c,a               ; Copy
        srl   c                 ; Shift to the right one pixel
        or    c                 ; A = bold version of pixel data
        ld    (hl),a            ; Write to VRAM
        inc   h                 ; Next pixel line
        inc   de                ; Next font line
        djnz  .l1
        pop   de
        pop   bc
        pop   hl
        ret

