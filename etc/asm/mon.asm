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
        
key:    push  hl
        push  bc
        push  de
        call  waits             ; Pause for a while
waitk:  ld    a,(23611)         ; Look at flags
        bit   5,a
        jr    z,waitk           ; No key pressed
        res   5,a               ; Reset flag
        ld    (23611),a
        pop   de
        pop   bc
        pop   hl
        ret
        
getkey: call  key
        ld    a,(23560)         ; Look at last-k
        call  prtchr
        ret
        
opench2 ld    a,2               ; Opens channel 'S' for printing
        call  $1601
        ret
        
prtchr  push  af
        push  af
        xor   a
        ld    (23692),a
        pop   af
        rst   $10
        pop   af
        ret
        
prtstr  ld    a,(de)            ; Get character
        cp    '$'               ; Is this the end of a string?
        ret   z                 ; Yes!
        call  prtchr            ; Print it
        inc   de
        jr    prtstr

fstart  ld    sp,stack
        call  opench2
        ld    de,home
        call  prtstr
        ld    de,welcome
        call  prtstr
        call  crlf
        
vstart  ld    sp,stack
        ld    a,8
        ld    (23658),a         ; Caps on
        ld    hl,errsp
        ld    (hl),low(vstart)
        inc   hl
        ld    (hl),high(vstart)
        dec   hl
        ld    (23613),hl
        
initv   ld    hl,initv
        push  hl
        call  crlf
        ld    a,'>'
        call  prtchr
        
start   call  getkey
        sub   'A'               ; Is it in the alphabet
        ret   c                 ; No
        cp    'S'-'A'+1
        ret   nc                ; No
        add   a,a               ; *2
        ld    hl,vectbl
        ld    e,a
        ld    d,0
        add   hl,de
        ld    e,(hl)
        inc   hl
        ld    d,(hl)
        ex    de,hl             ; HL = vector
        jp    (hl)
        
vectbl  dw    error
        dw    error
        dw    error
        dw    dump              ; D = dump
        dw    modify            ; E = edit
        dw    fill              ; F = fill
        dw    goto              ; G = goto
        dw    hunt              ; H = hunt
        dw    ident             ; I = identify filename
        dw    error
        dw    error
        dw    loadbytes         ; L = load
        dw    move              ; M = move
        dw    error
        dw    error
        dw    pump              ; P = print dump
        dw    error
        dw    chregs            ; R = change registers
        dw    savebytes         ; S = save
        
waits:  ld    bc,$8000
        ld    de,$4000
        ld    hl,$4000
        ldir
        ret
       
        ; Edit 
modify:
        call  getexpr1          ; Get start address
modifb  call  hexod             ; Output start address
        ld    a,' '
        call  prtchr
        ld    a,(hl)
        call  hexo
        ld    a,' '
        call  prtchr
        push  hl
        call  hexi
        pop   hl
        ld    (hl),a
        inc   hl
        call  crlf
        jr    modify            ; Loop until forced out by an error
        
getexpr2:
        call  hexd
        push  hl                ; Save E1
        ld    a,' '
        call  prtchr
        call  hexd
        push  hl
        pop   de                ; Save E2
        pop   hl                ; Get E1
        ret                     ; E1 = HL, E2 = DE
        
getexpr3:
        call  getexpr2
        push  hl
        push  de
        call  getexpr1
        pop   de
        pop   hl
        ret
        
getexpr1:
        ld    a,' '
        call  prtchr
        call  hexd
        push  hl
        pop   bc
        call  crlf
        ret
        
loadbytes:
        ld    a,' '
        call  prtchr
        call  getexpr2
        
        ld    (destt),hl
        
        ld    a,e
        or    d
        jp    z,errors
        
        ld    (lentt),de
        
        ld    de,loadmess
        call  prtstr
        
        ld    de,filename
        call  prtstr
        
getfile call  headin

        ld    de,header+1
        ld    hl,filename
        ld    b,10
        
compf   ld    a,(de)
        res   5,a
        ld    c,(hl)
        res   5,c
        cp    c
        jr    nz,getfile
        
        inc   hl
        inc   de
        djnz  compf
        
load1f  ld    de,(lentt)
        ld    hl,(destt)
        call  loadb
        ret
        
destt   dw    0
lentt   dw    0

cr      equ   $0d               ; Return
lf      equ   $0a               ; Linefeed
welcome db    cr,"*SBUG*"
        db    " (C) John Wilson"
        db    " 1984."
        db    cr,'$'
savemess:
        db    cr,"Press any key when ready$"
loadmess:
        db    cr,"Waiting for $"
notmess db    cr,"ROUTINE NOT IMPLEMENTED$"
errmess db    cr,"**ERROR**",cr,'$'
home    db    22,1,1,cr,'$'

savebytes:
        ld    a,' '
        call  prtchr
        call  getexpr2          ; Get 2 values: start and size
        ld    (header+13),hl    ; Param 1
        ld    a,e
        or    d
        jp    z,errrors
        ld    (header+11),de    ; Length
        
        ld    de,savemess
        call  prtstr
        
        call  waits
        call  waits
        call  waits
        call  getkey
        
        ld    a,3
        ld    de,header
        ld    hl,filename
        ld    (de),a
        inc   de
        
        ld    bc,10
        ldir
        call  headout
        call  waits
        call  waits
        call  waits
        ld    ix,(header+13)
        ld    de,(header+11)
        call  saveb
        ret
        
hexas:  push  hl
        call  hexo
        ld    a,' '
        call  prtchr
        pop   hl
        ret
        
line:   ld    b,8
nbyte   ld    a,(hl)
        call  hexas
        inc   hl
        djnz  nbyte
        call  crlf
        
dump
        ld    a,' '
        call  prtchr
        call  getexpr1
alock   ld    c,8
block   call  hexod
        ld    a,' '
        call  prtchr
        call  prtchr
        call  line
        dec   c
        jr    nz,block
        call  crlf
        call  crlf
        call  getkey
        cp    cr
        jr    z,alock
        ret
        
pine    ld    b,21
pbyte   ld    a,(hl)
        cp    32
        jr    sboggy-2
        cp    128
        jr    c,sboggy
        ld    a,'.'
sboggy  call  prtchr
        inc   hl
        djnz  pbyte
        call  crlf
        ret
        
pump:
        ld    a,' '
        call  prtchr
        call  getexpr1
palock  ld    c,8
plock   call  hexod
        ld    a,' '
        call  prtchr
        call  prtchr
        call  pine
        dec   c
        jr    nz,plock
        call  crlf
        call  crlf
        call  getkey
        cp    cr
        jr    z,palock
        ret
        
        
error:  nop
notimp: push  de
        ld    de,notmess
        call  prtstr
        pop   de
        ret
        
; Routines
; HEXO  Output hex number in accumulator
; HEXOD Output hex word in HL
; HEXI  Input hex number, put in accumulator
; HEXD  Input hex word, put in HL

