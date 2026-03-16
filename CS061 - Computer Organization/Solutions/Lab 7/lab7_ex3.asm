;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 7, ex 3
; Lab section: 24
; TA: Samuel Tapia 
; 
;=================================================



; test harness
.orig x3000
				 AND R0, R0, #0
				 ADD R0, R0, #5
				 
				 LD R6, STACK_ADDR
				 LD R5, FACT_ADDR
				 JSRR R5
				 
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:
FACT_ADDR       .FILL x3100
STACK_ADDR      .FILL xFE00
;===============================================================================================
.end


;-----------------------------------------------------------------------------------------------
; subroutines:  
;           R6 is the stack used to back up and restore the register
.orig x3100

FACT        ADD R6, R6, #-1
            STR R7, R6, #0
            ADD R6, R6, #-1
            STR R1, R6, #0
            
            ADD R1, R0, #-1
            BRz DONE
            
            ADD R1, R0, #0
            ADD R0, R1, #-1
            JSR FACT                        ;we can treat this jsr as br() #-3 
            LD R5, MUL_ADDR
            JSRR R5
            
DONE        LDR R1, R6, #0
            ADD R6, R6, #1
            LDR R7, R6, #0
            ADD R6, R6, #1
            RET
            
MUL_ADDR            .FILL x3200
.END


;-----------------------------------------------------------------------------------------------
; subroutines:
.orig x3200

MUL         ADD R6, R6, #-1
            STR R7, R6, #0
            ADD R6, R6, #-1
            STR R2, R6, #0

         
            ADD R2, R0, #0
            AND R0, R0, #0
            
LOOP        ADD R0, R0, R1
            ADD R2, R2, #-1
            BRp LOOP
            
            LDR R2, R6, #0
            ADD R6, R6, #1
            LDR R7, R6, #0
            ADD R6, R6, #1
            RET


.END

;-----------------------------------------------------------------------------------------------


