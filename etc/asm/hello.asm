; Hello World!

        org   $8000
        
OPEN    equ   5633
PRINT   equ   8252
        
Start:
        ld    a,2               ; Open channel #2
        call  OPEN
        ld    de,text           ; Print it
        ld    bc,textend-text
        jp    PRINT
        
text    db    "Hello, World!",13
textend:

        