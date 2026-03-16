;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 4, ex 2
; Lab section: 
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
LD R1, ARRAY_SIZE
LD R2, ARRAY_PTR
LDR R3, R2, #10         ;get the starting number 0, offset 10 is the first address after blkw

;--------------------------------------------------------------------------------------------------------step 1, sotre 0 - 9 into array (0 is hard code)
LOOP_INPUT
    STR R3, R2, #0      ;store r3's value into the address where r2 is pointing to.
    ADD R3, R3, #1      ;input through 0 to 9 
    ADD R2, R2, #1      ;move the address to the next
    
    ADD R1, R1, #-1     ;count down(array size)
    BRp LOOP_INPUT      ;if r1 is positive, keep looping 

;--------------------------------------------------------------------------------------------------------step 2, add all the numbers in the array with hex x30, and output it
LD R1, ARRAY_SIZE
LD R2, ARRAY_PTR
LD R3, Hex_30

LOOP_OUTPUT
    LDR R0, R2 #0
    ADD R0, R0, R3
    OUT
    
    ADD R2, R2 #1
    ADD R1, R1 #-1
    BRp LOOP_OUTPUT




HALT
;-----------------------
;Local data
;-----------------------
ARRAY_PTR       .FILL x4000
ARRAY_SIZE      .FILL #10
Hex_30          .FILL x30
.END


;-----------------------
;Remote data
;-----------------------
.ORIG x4000
ARRAY           .BLKW #10
NUM_0           .FILL #0

.END