;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 4, ex 1
; Lab section: 
; TA: Samuel Tapia
; 
;=================================================


.ORIG x3000
LD R1, ARRAY_SIZE
LD R2, ARRAY_PTR
LDR R3, R2, #10         ;get the starting number 0, offset 10 is the first address after blkw

;--------------------------------------------------------------------------------------------------------step 1, sotre 0 - 9 into array (0 is hard code)
LOOP
    STR R3, R2, #0      ;store r3's value into the address where r2 is pointing to.
    ADD R3, R3, #1      ;input through 0 to 9 
    ADD R2, R2, #1      ;move the address to the next
    
    ADD R1, R1, #-1     ;count down(array size)
    BRp LOOP            ;if r1 is positive, keep looping 



;--------------------------------------------------------------------------------------------------------step 2, grab the 7th value into R2
LD R1, ARRAY_PTR        ;set r1 as array_ptr
LDR R2, R1, #6          ;get the 7th value of the array. Because r1 is the first value in the array, so offset #6 can get the target number.

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
NUM_0           .FILL #0

.END