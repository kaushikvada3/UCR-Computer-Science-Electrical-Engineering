;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Assignment name: Assignment 3
; Lab section: 024  
; TA: Karan and Cody
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
LD R6, Value_ptr		
LDR R1, R6, #0			
;-------------------------------
;INSERT CODE STARTING FROM HERE
;--------------------------------
; Initialize loop counter in R5 to 16
AND R5, R5, #0
LD R5, SIXTEEN

; Initialize bit counter for spacing in R4
AND R4, R4, #0

AND R6, R6, #0
ADD R6, R6, #3


PRINT_LOOP:
    ADD R3, R1, #0
    BRzp PRINT_ZERO      ; If MSB is 0, jump to PRINT_ZERO

    ; Output '1'
    LD R0, CHAR_ONE
    OUT
    BRnzp NEXT_BIT

PRINT_ZERO
    ; Output '0'
    LD R0, CHAR_ZERO
    OUT

NEXT_BIT:
    ; Increment bit counter and check if it's time to add a space
    ADD R4, R4, #1
    AND R2, R4, #3 
    BRz PRINT_SPACE
    BR SHIFT_BIT

PRINT_SPACE:
    ADD R6, R6, #0
    BRnz SHIFT_BIT
    ADD R6, R6, #-1
    LD R0, CHAR_SPACE
    OUT
    AND R4, R4, #0      ; Reset bit counter after printing space
    

SHIFT_BIT:
    ADD R1, R1, R1       ; Left shift R1
    ADD R5, R5, #-1     ; Decrement the loop counter
    BRp PRINT_LOOP      ; Continue if not done with all bits
    
LD R0, NEWLINE
OUT

HALT

CHAR_ONE    .FILL    x0031    ; ASCII for '1'
CHAR_ZERO   .FILL    x0030    ; ASCII for '0'
CHAR_SPACE  .FILL    x0020    ; ASCII for space
SIXTEEN     .FILL    #16
NEWLINE     .FILL    #10

;---------------	
;Data
;---------------
Value_ptr	.FILL xCA01	; The address where value to be displayed is stored
.END

.ORIG xCA01					; Remote data
Value .FILL xABCD			; <----!!!NUMBER TO BE DISPLAYED AS BINARY!!!
;---------------	
;END of PROGRAM
;---------------	
.END
