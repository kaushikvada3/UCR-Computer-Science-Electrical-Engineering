;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 3. ex 2
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================


.ORIG x3000
;----------------------
;Program code
;----------------------
LD R6, DATA_PTR
LD R7, COUNT_DOWN

DO_WHILE_INPUT          ;input loop
    LEA R0, NOTE_1        
    PUTS                ;print the note_1
    
    GETC                ;get the input number
    OUT
    
    STR R0, R6, #0      ;store the input into the address where R6 is pointing to
    ADD R6, R6, #1      ;add 1 to the R6, everytime after we store a character
    
    LD R0, Newline      ;newline
    OUT
    
    ADD R7, R7, #-1     ;count down
    BRp DO_WHILE_INPUT  
    
LD R0, Newline          ;newline
OUT 

HALT
    
;----------------------
;Local Data
;----------------------
NOTE_1        .STRINGZ      "ENTER A CHARACTER: \n"
DATA_PTR    .FILL x4000
NewLine     .FILL x0A
COUNT_DOWN  .FILL #10

.END

.ORIG   X4000               ;Remote data located at address x4000
;----------------------
; Remote Data
;----------------------
Array_1     .BLKW   #10
    
.END