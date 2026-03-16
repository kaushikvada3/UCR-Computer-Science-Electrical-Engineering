;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 3. ex 1
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================


.ORIG x3000
;----------------------
;Program code
;----------------------

LD R5, DATA_PTR

LDR R4, R5, #0      ;get the date stored at x4000      
ADD R4, R4, #0      ;r4 <-- r4 + 0
STR R4, R5, #0      ;store the current r4 value into x4000

LD R5, DATA_PTR
LDR R4, R5, #1      ;get the date stored at x4001
ADD R4, R4, #1      ;r4 <-- r4 + 0
STR R4, R5, #1      ;store the current r4 value into x4000

HALT
    
;----------------------
;Local Data
;----------------------
DATA_PTR    .FILL x4000

.END
