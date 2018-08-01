; JET-PAC challenge by Adrian Brown.

        opt   Start:Start

        load  "rom.asm"

Start:
        ; Set screen to black
        xor   a
        out   ($fe),a
        ld    hl,$4000
        ld    de,$4001
        ld    bc,6911
        ld    (hl),a
        ldir

        ; Render text
        ld    a,2
        call  OPEN
        ld    de,text
        ld    bc,textend-text
        call  PRINT

        ld    a,0
        call  OPEN
        ld    de,text2
        ld    bc,text2end-text2
        call  PRINT
        
        jr    $

text:
        db    17,0,16,7,22,0,3,"1UP"
        db    22,0,15,16,5,"HI"
        db    22,0,27,16,7,"2UP"
        db    22,1,1,16,6,"000000"
        db    22,1,13,16,6,"000000"
        db    22,1,25,16,6,"000000"
        db    22,4,6,16,7,"JETPAC GAME SELECTION"
        db    22,9,6,"2   2 PLAYER GAME"
        db    22,13,6,"4   KEMPSTON JOYSTICK"
        db    22,19,6,"5   START GAME"
        db    18,1,22,7,6,"1   1 PLAYER GAME"
        db    22,11,6,"3   KEYBOARD"

textend:

text2
        db    17,0,16,7,22,1,0,127,"1983 A.C.G. ALL RIGHTS RESERVED"
text2end