;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 6, ex 3
; Lab section: 
; TA: Samuel Tapia 
; 
;=================================================

; test harness
.orig x3000
            LD R1, SUB_STACK_PUSH
            LD R2, SUB_STACK_POP
            LD R3, SUB_RPN_ADD
            
            LD R4, Base
            LD R5, MAX
            LD R6, TOS
            
            LEA R0, first_input_msg
            PUTS
			JSRR R1
			
            LEA R0, second_input_msg
            PUTS
			JSRR R1
			
			LEA R0, symbol_input_msg            ;only test multiplication here, so discard the input.
            PUTS
            GETC
            OUT
            
            JSRR R3
            
		    
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:
Base        .FILL xA000     ;A pointer to the bottom of the stack 
Max         .FILL xA005     ;The "highest" available address in the stack 
TOS         .FILL xA000     ;A pointer to the current Top Of the Stack 
first_input_msg     .stringz "\nPlease enter the first digit: \n"
second_input_msg    .stringz "\nPlease enter the second digit: \n"
symbol_input_msg    .stringz "\nPlease enter the operation symbol: \n"

SUB_STACK_PUSH      .FILL x3200
SUB_STACK_POP       .FILL x3400
SUB_RPN_ADD         .FILL x3600
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
; (1) back up R1, R2,r3, R7
                ST R1, backup_r1_3200
                ST R2, backup_r2_3200
                ST R3, backup_r3_3200
                ST R7, backup_r7_3200
                
; (2) Verify that TOS is less than MAX (if not, print Overflow message & quit)
                
                LD R1, Hex_30_3200
                NOT R1, R1
                ADD R1, R1, #1          ;R1 == -48
                
				NOT R2, R6
				ADD R2, R2, #1              ;r2 hold the negative number of tos. if r2 + r5 is positive(not include 0) means not overflow
				
				ADD R2, R2, R5
				BRp #7              
				    LEA R0, Overflow_Error  ;output the error msg, and reset the data 
				    PUTS
				    LD R1, backup_r1_3200
				    LD R2, backup_r2_3200 
				    LD R3, backup_r3_3200 
				    LD R7, backup_r7_3200
				    ret
			    
			    ADD R6, R6, #1              ;store the input, move the TOS to the next, increase first!
			    GETC
			    OUT
			    ADD R0, R1, R0              ;change r0 from number character to actual number
			    STR R0, R6, #0              ;write next
	 
; (3) restore backed up registers
                	LD R1, backup_r1_3200
				    LD R2, backup_r2_3200 
				    LD R3, backup_r3_3200 
				    LD R7, backup_r7_3200
	 
ret
;-----------------------------------------------------------------------------------------------
; SUB_STACK_PUSH local data
backup_r1_3200      .blkw #1
backup_r2_3200      .blkw #1
backup_r3_3200      .blkw #1
backup_r7_3200      .blkw #1
Overflow_Error      .stringz    "\n Stack Overflow! \n"

Hex_30_3200            .FILL x30  ; #48
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
; Return Value: R0 ← value popped off the stack
;		   R6 ← updated TOS
;------------------------------------------------------------------------------------------
.orig x3400

; (1) back up R1, R2, R7
                ST R1, backup_r1_3400
                ST R2, backup_r2_3400
                ST R3, backup_r3_3400 
                ST R7, backup_r7_3400
                
; (2) Verify that TOS is higher than BASE (if not, print Underflow message & quit)                
                NOT R2, R4
				ADD R2, R2, #1              ;r2 hold the negative number of base. if r2 + r6 is positive(not include 0) means not underflow
				
				ADD R2, R2, R6
				BRp #7              
				    LEA R0, Underflow_Error ;output the error msg, and reset the data 
				    PUTS
                    LD R1, backup_r1_3400
				    LD R2, backup_r2_3400 
				    LD R3, backup_r3_3400 
				    LD R7, backup_r7_3400
				    ret
                
                LDR R0, R6, #0              ;read first! 
                ADD R6, R6, #-1
                
; (3) restore backed up registers
                LD R1, backup_r1_3400
				LD R2, backup_r2_3400 
				LD R3, backup_r3_3400 
				LD R7, backup_r7_3400
                
ret
;-----------------------------------------------------------------------------------------------
; SUB_STACK_POP local data
backup_r1_3400      .blkw #1
backup_r2_3400      .blkw #1
backup_r3_3400      .blkw #1
backup_r7_3400      .blkw #1
Underflow_Error      .stringz    "\n Underflow Overflow! \n"

;===============================================================================================
.end


;------------------------------------------------------------------------------------------
; Subroutine: SUB_RPN_ADD
; Parameter (R4): BASE: A pointer to the base (one less than the lowest available
;                       address) of the stack
; Parameter (R5): MAX: The "highest" available address in the stack
; Parameter (R6): TOS (Top of Stack): A pointer to the current top of the stack
; Postcondition: The subroutine has popped off the top two values of the stack,
;		    added them together, and pushed the resulting value back
;		    onto the stack.
; Return Value: R6 ← updated TOS address
;------------------------------------------------------------------------------------------
.orig x3600
				 
; (1) back up R1, R2, R7
                ST R1, backup_r1_3600
                ST R2, backup_r2_3600
                ST R3, backup_r3_3600
                ST R7, backup_r7_3600	 
				 
; (2) pop the top two values off the stack

                JSRR R2                 ;r0 hold the first input
                AND R3, R3, #0          ;clear r3, and store the first number in R3
                ADD R3, R0, R3
                BRz #10                                                                                          

                JSRR R2                 ;now the second value is stored in r0
                BRnp #2                 ;if the first input is 0, result is 0                                   
                    AND R3, R3, #0      ;becauee one of first input number is 0, so just set both number as 0.
                    BRz #6
                    
                AND R1, R1, #0
                ADD R1, R1, R3          ;NOW r1 sotre the first popped value, used in the loop_add 
                
                LOOP_add                ;do the multiplication of those 2 numbers
                    ADD R0, R0, #-1
                        brnz #2
                    ADD R3, R3, R1
                    BRp LOOP_add
                
                LEA R0, result_msg
                PUTS
                
                AND R0, R0, #0          ;move the number from r3 to r0
                ADD R0, R3, R0     
                
                ADD R6, R6, #1          ;move the tos to the next address
                STR R0, R6, #0          ;store the value into the stack
                
                AND R0, R0, #0          ;move the number from r3 to r0
                ADD R0, R3, R0
                LD R1, Hex_30
                ADD R0, R0, R1
                OUT
                
; (3) restore backed up registers
                LD R1, backup_r1_3600
				LD R2, backup_r2_3600 
				LD R3, backup_r3_3600 
				LD R7, backup_r7_3600	 
ret
;-----------------------------------------------------------------------------------------------
; SUB_RPN_MULTIPLY local data
backup_r1_3600      .blkw #1
backup_r2_3600      .blkw #1
backup_r3_3600      .blkw #1
backup_r7_3600      .blkw #1

Hex_30              .FILL x30  ; #48
result_msg          .stringz "\n The final result is: \n"
;===============================================================================================
.end


; SUB_ADDED		

; SUB_GET_NUM		

; SUB_PRINT_DIGIT		Only needs to be able to print 1 digit numbers. 



