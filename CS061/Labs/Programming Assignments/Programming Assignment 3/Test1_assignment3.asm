;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik VAda
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
LD R6, Value_ptr		; R6 <-- pointer to value to be displayed as binary
LDR R1, R6, #0			; R1 <-- value to be displayed as binary 
;-------------------------------
;INSERT CODE STARTING FROM HERE
;--------------------------------
LD R2, COUNT           ; counting variable
;LD R3, MASK            
LD R4, SPACE_COUNTER   ; Load space counter for managing spaces after every 4 bits

PRINT_LOOP:
    ;AND R5, R1, R3  
    ADD R3,R2,#0
    ;BRz END_PRINTING
    
    BRzp PRINT_ZERO     ; If MSB is zero, jump to PRINT_ZERO
    BRn  PRINT_ONE
    ;LEA R0, ONE        ; Load address of the '1' character
    ;PUTS               ; Print '1'
   ; BRnzp SHIFT_AND_CONTINUE ; Jump to shift and continue
END_PRINTING
PRINT_ZERO
    LD R0, ZERO       ; Load address of the '0' character
    OUT  ; Print '0'
    BRnzp SHIFT_AND_CONTINUE
    
PRINT_ONE
    LD R0, ONE       ; Load address of the '0' character
    OUT  ; Print '0'
    BRnzp SHIFT_AND_CONTINUE

SHIFT_AND_CONTINUE:
    ADD R1, R1, R1     ; Left shift R1 to move the next bit into MSB position
   ; ADD R4, R4, #-1    ; Decrement the space counter
    ;BRz PRINT_SPACE    ; Print space after every 4 bits except the last group
    BRnzp NEXT_BIT     ; Skip space printing for now

;PRINT_SPACE:
   ; LD R0, SPACE      ; Load the space character
    ;OUT               ; Print space
    ;LD R4, SPACE_COUNTER   ; Reset space counter to 4

NEXT_BIT:
    ADD R2, R2, #-1    ; Decrement the loop counter
    BRp PRINT_LOOP     ; If there are bits left, loop again

HALT

COUNT          .FILL       #16        ; Number of bits in a 16-bit binary number
;MASK           .FILL       #0x8000    ; Mask for isolating the MSB
SPACE_COUNTER  .FILL       #4         ; Counter to manage spaces every 4 bits
ONE            .FILL    x30
ZERO           .FILL    x31
SPACE          .FILL    x20
;---------------	
;Data
;---------------
Value_ptr	.FILL xCA01	; The address where value to be displayed is stored

.END

.ORIG xCA01					; Remote data
Value .FILL xABCD			; <----!!!NUMBER TO BE DISPLAYED AS BINARY!!! Note: label is redundant.
;---------------	
;END of PROGRAM
;---------------	
.END
