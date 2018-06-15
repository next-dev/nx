; 8-bit Complementary-Multiple-With-Carry (CMWC) random number generator.
; Created by Patrik Rak in 2012, and revised in 2014/2015
; with optimisation contribution from Einar Saukas and Alan Albrecht.
; See http://www.worldofspectrum.org/forums/showthread.php?t=39632

        call  rnd
        ld    c,a
        ld    b,0
        ret

rnd     ld    hl,table

        ld    a,(hl)            ; i = (i & 7) + 1
        and   7
        inc   a
        ld    (hl),a

        inc   l                 ; hl = &cy

        ld    d,h               ; de = &q[i]
        add   a,l
        ld    e,a


        ld    a,(de)            ; y = q[i]
        ld    b,a
        ld    c,a
        ld    a,(hl)            ; ba = 256 * y + cy

        sub   c                 ; ba = 255 * y + cy
        jr    nc,$+3
        dec   b

        sub   c                 ; ba = 254 * y + cy
        jr    nc,$+3
        dec   b

        sub   c                 ; ba = 253 * y + cy
        jr    nc,$+3
        dec   b

        ld    (hl),b            ; cy = ba >> 8, x = ba & 255
        cpl                     ; x = (b-1) - x = -x - 1 = ~x + 1 - 1 = ~x
        ld    (de),a            ; q[i] = x

        ret

table   db    0,0,82,97,120,111,102,116,20,15

