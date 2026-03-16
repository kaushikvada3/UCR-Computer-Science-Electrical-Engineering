;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 1, ex 1
; Lab section: 1
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
;;;;;;;;;;;
;Program code
;;;;;;;;;;;

    LD R1, DEC_0    ;SUM         
    LD R2, DEC_12   ;m
    LD R3, DEC_6    ;i = n (here is 6)
    
    DO_WHILE_LOOP
            ADD R1, R1, R2
            ADD R3, R3, #-1
            BRP DO_WHILE_LOOP    ;only check the last edited register (here is R3)
            
    END_DO_WHILE_LOOP
    
    HALT

;;;;;;;;;;;
;Local Data
;;;;;;;;;;;

    DEC_0   .FILL #0
    DEC_6   .FILL #6
    DEC_12   .FILL #12
    
    
.END