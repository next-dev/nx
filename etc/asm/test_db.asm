; Test code to test data breakpoints

        opt   Start:Start

Start:
        ld    hl,data
        ld    a,(hl)
        cpl
        inc   hl
        ld    (hl),a
        ret

data    db    42
