
        jp    noise
        
sound   ld    a,$10
        out   ($fe),a
        call  delay
        
        ld    a,7
        out   ($fe),a
        call  delay
        
        ret
        
delay   ld    b,d
        ld    c,e
loop    dec   bc
        ld    a,b
        or    c
        jr    nz,loop
        ret
        
dela    equ   100
durat   equ   100

noise   ld    de,dela
        ld    hl,durat
buzz    call  sound

        dec   de
        dec   hl
        ld    a,l
        or    h
        jr    nz,buzz
        
        ret
