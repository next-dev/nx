; Hex editor
; Based on code from Your Sinclair Program Pitstop (issue 46)

        org   25000
        opt   Start:Start

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
        ld    hl,$0700
        call  printString

        jp    $

titleString:
        db    "Hex Loader\0"

printString:
        