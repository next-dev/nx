GetPixelAddr:
        ld    a,e               ; Use copy of x coord
        rrca                    ; * 8
        rrca
        rrca
        and   31                ; Mask rotated in bits
        ld    h,a

        ld    a,d               ; Get Y
        rla                     ; Rotate y3-y5 into position
        rla
        and   224               ; And isolate
        or    h                 ; Merge into X copy

        ld    l,a               ; Store in L
        ld    a,d               ; Get Y
        rra                     ; Rotate y7-y6 into position
        rra
        or    128
        rra
        xor   d
        and   245
        xor   d
        ld    h,a
        ld    a,e               ; Get pixel position
        and   7
        ret
        