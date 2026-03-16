;=================================================
; Name: Kaushik Vada
; Email: kaushikvada3@gmail.com
; 
; Lab: lab 5, ex 1
; Lab section: 024  
; TA: Karan and Cody
; 
;=================================================
.orig x3000
; Initialize the stack. Don't worry about what that means for now.
ld r6, top_stack_addr ; DO NOT MODIFY, AND DON'T USE R6, OTHER THAN FOR BACKUP/RESTORE


; your code goes here

LEA R1, Char_Array
LD R4, SUB_GET_STRING_POINTER
JSRR R4
	
LEA R0, Char_Array
PUTS

halt

; your local data goes here

SUB_GET_STRING_POINTER .FILL x3200
Char_Array .BLKW #100

top_stack_addr .fill xFE00 ; DO NOT MODIFY THIS LINE OF CODE
.end

; your subroutines go below here

;------------------------------------------------------------------------
; Subroutine: SUB_GET_STRING
; Parameter (R1): The starting address of the character array
; Postcondition: The subroutine has prompted the user to input a string,
;	terminated by the [ENTER] key (the "sentinel"), and has stored 
;	the received characters in an array of characters starting at (R1).
;	the array is NULL-terminated; the sentinel character is NOT stored.
; Return Value (R5): The number of non-sentinel chars read from the user.
;	R1 contains the starting address of the array unchanged.
;-------------------------------------------------------------------------

.ORIG x3200

	;========================
	; Subroutine Instructions
	;========================

	SUB_GET_STRING

		ST R0, BACKUP_R0_3200
		ST R1, BACKUP_R1_3200
		ST R2, BACKUP_R2_3200
		ST R3, BACKUP_R3_3200
		ST R4, BACKUP_R4_3200
		ST R6, BACKUP_R6_3200
		ST R7, BACKUP_R7_3200
		
		; Get negative NEWLINE
		LD R2, NewLine
		NOT R2, R2
		ADD R2, R2, #1
		AND R5, R5, x0
		
		Input_Loop
			GETC
			OUT
			
			STR R0, R1, #0
			
			ADD R0, R0, R2
			BRz END_OF_LOOP
			
			ADD R1, R1, #1
			ADD R5, R5, #1
			
			BR Input_Loop
		END_OF_LOOP
		
		AND R0, R0, x0
		STR R0, R1, #0

		LD R0, BACKUP_R0_3200
		LD R1, BACKUP_R1_3200
		LD R2, BACKUP_R2_3200
		LD R3, BACKUP_R3_3200
		LD R4, BACKUP_R4_3200
		LD R6, BACKUP_R6_3200
		LD R7, BACKUP_R7_3200
		RET

	;========================
	; Subroutine Data
	;========================

	BACKUP_R0_3200 .BLKW #1
	BACKUP_R1_3200 .BLKW #1
	BACKUP_R2_3200 .BLKW #1
	BACKUP_R3_3200 .BLKW #1
	BACKUP_R4_3200 .BLKW #1
	BACKUP_R5_3200 .BLKW #1
	BACKUP_R6_3200 .BLKW #1
	BACKUP_R7_3200 .BLKW #1
	NewLine        .FILL x000A

.END

