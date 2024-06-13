;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 9, ex 2
; Lab section: 24
; TA: Samuel Tapia 
; 
;=================================================

; test harness
.orig x3000
			LEA R0, prompt
			puts
			GETC
			OUT

			ADD R2, R0, #0      ;R2 = INPUT CHAR.
			
			LD R1, SUB_COUNT_ONES_ptr
			JSRR R1             ;R1 = NUM OF 1'S 
			
			LEA R0, end_prompt_1
			PUTS
			ADD R0, R2, #0
			OUT
			LEA R0, end_prompt_2
			PUTS
			LD R2, HEX_30
			ADD R0, R1, R2
			OUT
			
				 
	 
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:
prompt          .stringz "Enter the character: \n"
end_prompt_1    .stringz "\nThe number of 1's in '"
end_prompt_2    .stringz "' is: "
HEX_30          .FILL x30
;--------------------
;subroutines_address
;--------------------
SUB_COUNT_ONES_ptr      .FILL x3200
;===============================================================================================
.end


; subroutines:

;------------------------------------------------------------------------------------------
; Subroutine: SUB_COUNT_ONES (Be sure to fill details in below)
;
; Parameter (R): R0(the user's input character)
; Return Value: R1(the number of 1's for the input character)
;------------------------------------------------------------------------------------------
.orig x3200
;BACKUP     
            ST R0, backup_r0_3200
            ST R2, backup_r2_3200    
            ST R3, backup_r3_3200     
            ST R4, backup_r4_3200
            ST R5, backup_r5_3200
            ST R6, backup_r6_3200  
            ST R7, backup_r7_3200 	 
		    
		    LD R3, num_16           ;r3 = 16
		    AND R1, R1, #0          ;USE R1 AS COUNTER
		    
		    check_loop
		        AND R3, R3, R3
		        BRnz end_sub        ;if r3 = 0, end the subroutine
		        
		        ADD R3, R3, #-1
		        AND R0, R0, R0
		        BRzp #1             ;if r0 is positive or 0, the LMD is 0
		            ADD R1, R1, #1
		        ADD R0, R0, R0      ;left shift
			    BR check_loop    
			            
			             
			end_sub
;restore    
            LD R0, backup_r0_3200   
            LD R2, backup_r2_3200     
            LD R3, backup_r3_3200
            LD R4, backup_r4_3200 
            LD R5, backup_r5_3200
            LD R6, backup_r6_3200 
            LD R7, backup_r7_3200				 
ret
;-----------------------------------------------------------------------------------------------
; SUB_COUNT_ONES local data
backup_r0_3200  
backup_r2_3200  .BLKW #1
backup_r3_3200  .BLKW #1
backup_r4_3200  .BLKW #1
backup_r5_3200  .BLKW #1
backup_r6_3200  .BLKW #1
backup_r7_3200  .BLKW #1

num_16          .FILL #16
.end

