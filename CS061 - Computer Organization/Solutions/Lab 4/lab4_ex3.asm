;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 4, ex 3
; Lab section: 
; TA: Samuel Tapia
; 
;=================================================


.ORIG x3000
LD R3, ARRAY_PTR        ;ptr to the array's first address
LD R4, ARRAY_SIZE       ;used for loop count down
LDR R5, R3, #10         ;starting number(1)

LOOP_INPUT
    STR R5, R3, #0
    ADD R3, R3, #1      ; move the address to the next
    ADD R5, R5, R5      ; R5 <-- R5 + R5 (double the r5) because r5 start with 2^0. so the following r5 will be 2^1, 2^2, 2^3 ....
    ADD R4, R4, #-1     ; count down
    BRp LOOP_INPUT
    
LD R3, ARRAY_PTR        ;ptr to the array's first address
LDR R2, R3, #6          ;the r3 is the first address of array. set the offset as 6. get the array[7]

HALT
;-----------------------
;Local data
;-----------------------
ARRAY_PTR       .FILL x4000
ARRAY_SIZE      .FILL #10
.END


;-----------------------
;Remote data
;-----------------------
.ORIG x4000
ARRAY           .BLKW #10
First_number    .FILL #1
.END