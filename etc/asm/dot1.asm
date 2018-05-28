; Dot file test

        ; This will generate a file called "border" next to this .asm.
        ; Current directory is the same as this file.
        opt   dot:"border"

        ld    a,2
        out   ($fe),a
        ret


