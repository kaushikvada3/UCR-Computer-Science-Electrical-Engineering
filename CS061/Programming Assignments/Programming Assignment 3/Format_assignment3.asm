;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: 
; Email: 
; 
; Assignment name: Assignment 3
; Lab section: 
; TA:
; 
; I hereby certify that I have not received assistance on this assignment,
; or used code, from ANY outside source other than the instruction team
; (apart from what was provided in the starter file).
;
;=========================================================================

.ORIG x3000			; Program begins here
;-------------
;Instructions
;-------------
LD R6, Value_ptr		; R6 <-- pointer to value to be displayed as binary
LDR R1, R6, #0			; R1 <-- value to be displayed as binary 
;-------------------------------
;INSERT CODE STARTING FROM HERE
;-------------------------------

; Initialize R2 as a loop counter with 16
AND R2, R2, #0          ; Clear R2
LD R2, SIXTEEN



PRINT_LOOP:
    AND R3, R1, #0x8000 ; Isolate the MSB of R1 (0x8000 = 32768 in decimal)
    BRz PRINT_ZERO      ; If MSB is 0, branch to PRINT_ZERO

    ; Output '1'
    LD R0, CHAR_ONE     ; Load the ASCII value for '1'
    OUT                 ; Print '1'
    BRnzp CONTINUE      ; Continue to next bit

PRINT_ZERO:
    ; Output '0'
    LD R0, CHAR_ZERO    ; Load the ASCII value for '0'
    OUT                 ; Print '0'

CONTINUE:
    ADD R1, R1, R1      ; Left shift R1 to prepare the next MSB
    ADD R2, R2, #-1     ; Decrement the loop counter
    BRp PRINT_LOOP      ; If there are bits left, repeat the loop

HALT
;---------------	
;Data
;---------------
Value_ptr	.FILL xCA01	; The address where value to be displayed is stored
CHAR_ONE    .FILL x0031 ; ASCII value for '1'
CHAR_ZERO   .FILL x0030 ; ASCII value for '0'
SIXTEEN .FILL #16

.END

.ORIG xCA01					; Remote data
Value .FILL xABCD			; <----!!!NUMBER TO BE DISPLAYED AS BINARY!!! Note: label is redundant.
;---------------	
;END of PROGRAM
;---------------	
.END

