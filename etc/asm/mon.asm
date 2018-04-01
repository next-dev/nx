;
; Debugger/Monitor from book "Cracking The Code"
; (C) John Wilson, 1984
;

        org   25500
        
R_SAVE  equ   $04c2
R_LOAD  equ   $0556
        
        jp    fstart
ALTER   equ   39
COMMA   equ   ','

saveb   ld    a,$ff
        ; IX points to start of block
        ; DE contains number of bytes
        jp    R_SAVE
        
loadb   scf
        ld    a,$ff
        jp    R_LOAD
        
        ; Header format:
        ;
        ; Byte Length Description
        ; 0    1      Type (0-3)
        ; 1    10     Filename
        ; 11   2      Length of data block
        ; 13   2      Parameter 1
        ; 15   2      Parameter 2
        ;
        
headin  ld    de,17
        ld    ix,header
        xor   a
        scf
        call  R_LOAD            ; Load the header
        
        ld    a,(header+11)     ; Store the LSB of length
        ld    c,a
        ld    a,'$'             ; Terminate the string for the print routine
        ld    (header+11),a
        call  crlf
        ld    de,header+1       ; Print the filename
        call  prtstr
        ld    a,' '
        call  prtchar
        
        ex    de,hl
        ld    (hl),c            ; Restore the LSB of length
        inc   hl
        inc   hl
        inc   hl                ; HL = MSB of param 1
        ld    a,(hl)
        call  hexo              ; Output MSB in hex
        dec   hl
        ld    a,(hl)
        call  hexo              ; Output LSB in hex
        dec   hl                ; HL = MSB of length
        
        ld    a,' '
        call  prtchr
        
        ld    a,(hl)            ; Output length
        call  hexo
        dec   hl
        ld    a,(hl)
        call  hexo
        
        call  crlf
        ret

headout ld    de,17
        ld    ix,header
        xor   a
        call  R_SAVE
        ret
        