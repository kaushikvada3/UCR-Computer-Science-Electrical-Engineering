;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Yu Guo
; Username: yguo139
; Email: yguo139@ucr.edu
; 
; Assignment name: Assignment 1
; TA: Samuel Tapia
; 
; I hereby certify that I have not received assistance on this assignment,
; or used code, from ANY outside source other than the instruction team
; (apart from what was provided in the starter file).
;
;=========================================================================


;---------------------------------------------------------
;REG VALUES         R0  R1      R2  R3  R4  R5  R6      R7 
;-------------------------------------------------------------
;Pre-loop           0   0       0   0   0   0   0       0
;iteration 01       0   6       12  0   0   0   0       0
;iteration 02       0   5       12  12  0   0   0       0
;iteration 03       0   4       12  24  0   0   0       0
;iteration 04       0   3       12  36  0   0   0       0
;iteration 05       0   2       12  48  0   0   0       0
;iteration 06       0   1       12  60  0   0   0       0    
;Last iteration     0   32767   12  72  0   0   12286   0





.ORIG x3000			; Program begins here

;-------------
;Instructions: 
;-------------
LD R1, DEC_6    ;R1 <--6
LD R2, DEC_12   ;R2 <--12
AND R3, R3, #0  ;replace LD R3, DEC_0. R3 <-- R3 + 0, Request: the defalut R3 has to be 0
                

;LD R3, DEC_0   ;R3 <--0,   another approach for AND R3, R3 #0,
;AND R3, R3 #0    &    LD R3, DEC_0 get the same output value here.

DO_WHILE ADD R3, R3, R2     ; R3 <-- R3 + R2
         ADD R1, R1, #-1    ; R1 <-- R1 - 1
         BRp DO_WHILE       ;if R1 is positive, goto DO_WHILE

HALT

;---------------
;Data
;---------------
DEC_0   .FILL #0    ;put the value 0 into memory here
DEC_6   .FILL #6    ;put the value 6 into memory here
DEC_12  .FILL #12   ;put the value 12 into memory here

;---------------	
;END of PROGRAM
;---------------	
.END
