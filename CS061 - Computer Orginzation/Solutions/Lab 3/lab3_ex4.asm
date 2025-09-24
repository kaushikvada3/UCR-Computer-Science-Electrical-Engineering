;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 3. ex 4
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
LD R1, DATA_PTR
LD R7, HEX_0A

;--------------------
;INPUT
;--------------------

DO_WHILE_INPUT
    LEA R0, NOTE
    PUTS
    
    GETC
    OUT
    
    STR R0, R1, #0              ;store the date into the address where r1 is pointing to.
    ADD R1, R1, #1              ;add 1 to r1, soit will pointing to the next mem address 
    
    NOT R6, R7                  ;1s complement of x0A
    ADD R6, R6, #1              ;2s complement of x0A, now R6 hold the value: -12
    
    ADD R6, R6, R0;             ;compare the input value(R0)'s ASCII code with R6(-x0A), if the sum = 0, end the loop
BRnp DO_WHILE_INPUT



;--------------------
;OUTPUT
;--------------------

LD R1, DATA_PTR                 ;reset r1 to the mem address x4000
LEA R0, NOTE_2
PUTS
DO_WHILE_OUTPUT
    NOT R6, R7                  ;1s complement of x0A
    ADD R6, R6, #1              ;2s complement of x0A, now R6 hold the value: -10
    
    LDR R0, R1, #0              ;store the date into the address where r1 is pointing to.
    ADD R1, R1, #1              ;add 1 to r1, soit will pointing to the next mem address 
    
    ADD R6, R6, R0              ;check if R0 is ASCII CODE is x0A
    BRz #1                      ;if R0's ASCII CODE is x0A, skip the next 2 steps
        OUT

BRnp DO_WHILE_OUTPUT
    

HALT
;----------------------
;Local Data
;----------------------
NOTE        .STRINGZ "\nEnter a character, or press enter key to stop\n"
NOTE_2      .STRINGZ "\n All the input characters are: \n"
DATA_PTR    .FILL x4000
HEX_0A      .FILL x0A           ;represent newline in ASCII code

.END

;----------------------
;Remote data
;----------------------
.ORIG X4000

.END 