;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 3. ex 3
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
LEA R0, NOTE_2        
PUTS                    ;print the note_2

LD R6, DATA_PTR         ;let r6 pointing to x4000(data_ptr)
LD R7, COUNT_DOWN       ;reset r7 to 10

DO_WHILE_OUTPUT         ;output loop
    LDR R0, R6, #0
    OUT
    
    LD R0, Newline      ;newline
    OUT
    
    ADD R6, R6, #1      ;add 1 to the R6, everytime after we OUTPUT a character
    ADD R7, R7, #-1     ;count down
    BRp DO_WHILE_OUTPUT  
    
    
LD R0, Newline      ;end with newline
OUT
    
HALT
    
;----------------------
;Local Data
;----------------------
NOTE_1        .STRINGZ "ENTER A CHARACTER: \n"
NOTE_2        .STRINGZ "THE CHARACTERS IN THE ARRAY ARE: \n"
DATA_PTR      .FILL x4000
NewLine       .FILL x0A
COUNT_DOWN    .FILL #10

.END

.ORIG   X4000           ;Remote data located at address x4000
;----------------------
; Remote Data
;----------------------
ARRAY_1       .BLKW   #10

.END