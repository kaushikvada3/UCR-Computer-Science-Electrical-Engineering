;=================================================
; Name: Kaushik Vada
; Email: kaushikvada3@gmail.com
; 
; Lab: lab 5, ex 3
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

;exercise 2
;capture the string
LD R0, PRINT_NEWLINE
OUT
	
LEA R0, PRINT_MESSAGE
PUTS
LEA R0, CHAR_ARRAY
PUTS
	
LD R4, SUB_IS_PALINDROME_POINTER
JSRR R4
	
ADD R4, R4, #0
BRp TRUE_CONDITION_CHECK
	
FALSE_CONDITION_CHECK
		LEA R0, FALSE_NOTE
		PUTS
		BR END_OF_CONDITIONS
TRUE_CONDITION_CHECK
		LEA R0, TRUE_NOTE
		PUTS
END_OF_CONDITIONS


halt

; your local data goes here

PRINT_MESSAGE .STRINGZ "The string \""
TRUE_NOTE .STRINGZ "\" IS a palindrome"
FALSE_NOTE .STRINGZ "\" IS NOT a palindrome"
PRINT_NEWLINE .FILL x000A

SUB_GET_STRING_POINTER .FILL x3200
SUB_IS_PALINDROME_POINTER .FILL X3400
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





;-------------------------------------------------------------------------
; Subroutine: SUB_IS_PALINDROME
; Parameter (R1): The starting address of a null-terminated string
; Parameter (R5): The number of characters in the array.
; Postcondition: The subroutine has determined whether the string at (R1)
;		 is a palindrome or not, and returned a flag to that effect.
; Return Value: R4 {1 if the string is a palindrome, 0 otherwise}
;-------------------------------------------------------------------------

.orig x3400

SUB_IS_PALINDROME

		ST R0, BACKUP_R0_3400
		ST R1, BACKUP_R1_3400
		ST R2, BACKUP_R2_3400
		ST R3, BACKUP_R3_3400
		ST R4, BACKUP_R4_3400
		ST R5, BACKUP_R5_3400
		ST R6, BACKUP_R6_3400
		ST R7, BACKUP_R7_3400
		
		ADD R2, R1, R5
		ADD R2, R2, #-1					; R2 <- Pointer to the last element
		
		WHILE
			MIDDLE_CHECK
                ADD R4, R2, #0      ; Copy R2 to R4
                NOT R4, R4          ; Invert R4
                ADD R4, R4, #1      ; R4 = -R2 (two's complement)
                ADD R4, R1, R4      ; R4 = R1 - R2
                BRzp TRUE_CONDITION  ; Branch if R1 >= R2
				
			LDR R0, R1, #0  ; Load character from R1 into R0
            LDR R3, R2, #0  ; Load character from R2 into R3
            NOT R3, R3      ; Invert R3
            ADD R3, R3, #1  ; R3 = -R3 (two's complement)
            ADD R0, R0, R3  ; R0 = R0 - R3 (compare characters)
            BRnp FALSE_CONDITION  ; Branch if characters are not equal


			ADD R1, R1, #1  ; Move start pointer to the next character
            ADD R2, R2, #-1 ; Move end pointer to the previous character
            BR WHILE   ; Repeat the loop

		END_WHILE
		
		FALSE_CONDITION
			AND R4, R4, x0
			BR END_OF_CONDITION
		TRUE_CONDITION
			AND R4, R4, x0
			ADD R4, R4, #1
		END_OF_CONDITION

		LD R0, BACKUP_R0_3400
		LD R1, BACKUP_R1_3400
		LD R2, BACKUP_R2_3400
		LD R3, BACKUP_R3_3400
		LD R5, BACKUP_R5_3400
		LD R6, BACKUP_R6_3400
		LD R7, BACKUP_R7_3400
		RET

	;========================
	; Subroutine Data
	;========================

	BACKUP_R0_3400 .BLKW #1
	BACKUP_R1_3400 .BLKW #1
	BACKUP_R2_3400 .BLKW #1
	BACKUP_R3_3400 .BLKW #1
	BACKUP_R4_3400 .BLKW #1
	BACKUP_R5_3400 .BLKW #1
	BACKUP_R6_3400 .BLKW #1
	BACKUP_R7_3400 .BLKW #1 
	SUB_TO_UPPER_PTR_3400 .FILL x3600

.END



;-------------------------------------------------------------------------
; Subroutine: SUB_TO_UPPER
; Parameter (R1): Starting address of a null-terminated string
; Postcondition: The subroutine has converted the string to upper-case
;     in-place i.e. the upper-case string has replaced the original string
; No return value, no output, but R1 still contains the array address, unchanged
;-------------------------------------------------------------------------

.ORIG x3600

	;========================
	; Subroutine Instructions
	;========================

	SUB_TO_UPPER

		ST R0, BACKUP_R0_3600
		ST R1, BACKUP_R1_3600
		ST R2, BACKUP_R2_3600
		ST R3, BACKUP_R3_3600
		ST R4, BACKUP_R4_3600
		ST R5, BACKUP_R5_3600
		ST R6, BACKUP_R6_3600
		ST R7, BACKUP_R7_3600
		
		LD R2, BITWISE_VALUE
		
		LOOP_TO_CONVERT
			LDR R0, R1, #0
			ADD R0, R0, #0
			BRz END_CONVERSION
			
			AND R0, R0, R2
			STR R0, R1, #0
			ADD R1, R1, #1
			BR LOOP_TO_CONVERT
		END_CONVERSION

		LD R0, BACKUP_R0_3600
		LD R1, BACKUP_R1_3600
		LD R2, BACKUP_R2_3600
		LD R3, BACKUP_R3_3600
		LD R4, BACKUP_R4_3600
		LD R5, BACKUP_R5_3600
		LD R6, BACKUP_R6_3600
		LD R7, BACKUP_R7_3600
		RET

	;========================
	; Subroutine Data
	;========================

	BACKUP_R0_3600 .BLKW #1
	BACKUP_R1_3600 .BLKW #1
	BACKUP_R2_3600 .BLKW #1
	BACKUP_R3_3600 .BLKW #1
	BACKUP_R4_3600 .BLKW #1
	BACKUP_R5_3600 .BLKW #1
	BACKUP_R6_3600 .BLKW #1
	BACKUP_R7_3600 .BLKW #1
	BITWISE_VALUE .FILL x005F

.END