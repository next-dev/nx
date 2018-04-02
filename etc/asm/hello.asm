; Hello World!

        opt   Start:Start

        org   $8000
        
        load  "rom.asm"
        
Start:
        ld    a,2               ; Open channel #2
        call  OPEN
        ld    de,text           ; Print it
        ld    bc,textend-text
        call  PRINT
        jp    $

text:   db    "Hello, World!\r-------------\r"
textend:


        