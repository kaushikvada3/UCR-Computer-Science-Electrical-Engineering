;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu   
; 
; Lab: lab 1, ex 1
; Lab section: 024 
; TA: Karan Bhogal & Cody Kurpanek
; 
;=================================================

.orig x3000 ;starting memory location

;declare (load) the variables
;basically what you are doing is you are adding the value 6, 12 times in order to perform 6*12, and this requires a DO_WHILE_LOOP for you to do so. 

LD R1, DEC_0 ;R1 starts off with zero (this is the "sum" variable)
LD R2, DEC_6 ;R2 is the incrementing variable
LD R3, DEC_12;R3 is the number of times the "self-adding" act is going to happen (THIS IS THE CONTROLLER VARIABLE)

DO_WHILE_LOOP
    ADD R1, R1, R2 ; R1 = R1 + R2 (basically the += operation in C++)
    ADD R3, R3, #-1
    BRp DO_WHILE_LOOP ;controls the loop based on as long as the controlling variable (R3) is positive (the p in BRp stands for positive)
END_DO_WHILE_LOOP

HALT ;stops the program

;Local data

DEC_0 .FILL #0  ;this is telling the assembler to fill the memory space with the value 0
DEC_6 .FILL #6 ;this is telling the assembler to fill the memory space with the value 6
DEC_12 .FILL #12 ;this is telling the assembler to full the memory space with the value 12

.END