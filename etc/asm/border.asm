Colour  equ   2 + 4*3 - 12

Start:
        ld    a,Colour
        out   ($fe),a
        ret
        