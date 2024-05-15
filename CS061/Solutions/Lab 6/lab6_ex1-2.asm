;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 6, ex 1 & 2
; Lab section: 
; TA: Samuel Tapia  
; 
;=================================================



; test harness
.orig x3000
            LD R4, Base
            LD R5, MAX
            LD R6, TOS
            
            LD R1, SUB_STACK_PUSH
            LD R2, SUB_STACK_POP
            
            
			JSRR R1
			JSRR R1
			JSRR R1
			JSRR R1
            JSRR R1
            JSRR R1
			
																						
			JSRR R2
			JSRR R2
			JSRR R2
			JSRR R2
			JSRR R2
			JSRR R2

			
			JSRR R1
            JSRR R1
            JSRR R1
            JSRR R1
            JSRR R1

																																	
			
		    
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:
Base        .FILL xA000     ;A pointer to the bottom of the stack 
Max         .FILL xA005     ;The "highest" available address in the stack 
TOS         .FILL xA000     ;A pointer to the current Top Of the Stack 

SUB_STACK_PUSH      .FILL x3200
SUB_STACK_POP       .FILL x3400
;===============================================================================================
.end

; subroutines:

;------------------------------------------------------------------------------------------
; Subroutine: SUB_STACK_PUSH
; Parameter (R0): The value to push onto the stack
; Parameter (R4): BASE: A pointer to the base (one less than the lowest available
;                       address) of the stack
; Parameter (R5): MAX: The "highest" available address in the stack
; Parameter (R6): TOS (Top of Stack): A pointer to the current top of the stack
; Postcondition: The subroutine has pushed (R0) onto the stack (i.e to address TOS+1). 
;		    If the stack was already full (TOS = MAX), the subroutine has printed an
;		    overflow error message and terminated.
; Return Value: R6 ← updated TOS
;------------------------------------------------------------------------------------------
.orig x3200

; (1) back up R1, R2, R7
                ST R1, backup_r1_3200
                ST R2, backup_r2_3200
                ST R7, backup_r7_3200
                
;Verify that TOS is less than MAX (if not, print Overflow message & quit)
				NOT R2, R6
				ADD R2, R2, #1              ;r2 hold the negative number of tos. if r2 + r5 is positive(not include 0) means not overflow
				
				ADD R2, R2, R5
				BRp #6              
				    LEA R0, Overflow_Error  ;output the error msg, and reset the data 
				    PUTS
				    LD R1, backup_r1_3200
				    LD R2, backup_r2_3200 
				    LD R7, backup_r7_3200
				    ret
			    
			    ADD R6, R6, #1              ;store the input, move the TOS to the next, increase first!
			    GETC
			    OUT
			    STR R0, R6, #0              ;write next
	 
; (3) restore backed up registers
                	LD R1, backup_r1_3200
				    LD R2, backup_r2_3200 
				    LD R7, backup_r7_3200
	 
ret
;-----------------------------------------------------------------------------------------------
; SUB_STACK_PUSH local data
backup_r1_3200      .blkw #1
backup_r2_3200      .blkw #1
backup_r7_3200      .blkw #1
Overflow_Error      .stringz    "\n Stack Overflow! \n"

;===============================================================================================
.end




;------------------------------------------------------------------------------------------
; Subroutine: SUB_STACK_POP
; Parameter (R4): BASE: A pointer to the base (one less than the lowest available                      
;                       address) of the stack
; Parameter (R5): MAX: The "highest" available address in the stack
; Parameter (R6): TOS (Top of Stack): A pointer to the current top of the stack
; Postcondition: The subroutine has popped MEM[TOS] off of the stack.
;		    If the stack was already empty (TOS = BASE), the subroutine has printed
;                an underflow error message and terminated.
; Return Value: R0 ← value popped of the stack
;		   R6 ← updated TOS
;------------------------------------------------------------------------------------------
.orig x3400

; (1) back up R1, R2, R7
                ST R1, backup_r1_3400
                ST R2, backup_r2_3400
                ST R7, backup_r7_3400
                
;Verify that TOS is higher than BASE (if not, print Underflow message & quit)                
                NOT R2, R4
				ADD R2, R2, #1              ;r2 hold the negative number of base. if r2 + r6 is positive(not include 0) means not underflow
				
				ADD R2, R2, R6
				BRp #6              
				    LEA R0, Underflow_Error ;output the error msg, and reset the data 
				    PUTS
                    LD R1, backup_r1_3400
				    LD R2, backup_r2_3400 
				    LD R7, backup_r7_3400
				    ret
                
                LDR R0, R6, #0              ;read first! 
                ADD R6, R6, #-1
                
; (3) restore backed up registers
                LD R1, backup_r1_3400
				LD R2, backup_r2_3400 
				LD R7, backup_r7_3400
                
ret
;-----------------------------------------------------------------------------------------------
; SUB_STACK_POP local data
backup_r1_3400      .blkw #1
backup_r2_3400      .blkw #1
backup_r7_3400      .blkw #1
Underflow_Error      .stringz    "\n Underflow Overflow! \n"

;===============================================================================================
.end
