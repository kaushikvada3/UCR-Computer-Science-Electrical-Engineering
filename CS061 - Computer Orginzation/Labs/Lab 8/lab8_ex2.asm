;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Lab: lab 8, ex 2
; Lab section: 024
; TA: Karan and Cody
; 
;=================================================

.orig x3000

; test harness
    LEA R0, prompt
    puts
    GETC
    OUT

    ADD R2, R0, #0      ;R2 = INPUT CHAR.
    
    LD R1, PARITY_CHECK_3600_PTR
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
PARITY_CHECK_3600_PTR  .FILL    x3200
;===============================================================================================
.end


; subroutines:

;------------------------------------------------------------------------------------------
; Subroutine: PARITY_CHECK_3600
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
		    
		    LD R3, num_16           ; Load the value 16 into R3 to use as a loop counter for 16-bit input.
            AND R1, R1, #0          ; Clear R1 to use it as a counter for the number of '1' bits.
            
            check_loop
                AND R3, R3, R3      ; Check if R3 is zero.
                BRnz end_sub        ; If R3 is zero, branch to end_sub to exit the loop.
            
                ADD R3, R3, #-1     ; Decrement the loop counter R3 by 1.
                
                AND R0, R0, R0      ; Check the least significant bit of R0.
                BRzp #1             ; If the least significant bit is 0 (R0 is positive or zero), skip the next instruction.
                    ADD R1, R1, #1  ; If the least significant bit is 1, increment the counter R1.
            
                ADD R0, R0, R0      ; Left shift R0 to check the next bit in the next iteration.
                BR check_loop       ; Repeat the loop for the next bit.
            
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
