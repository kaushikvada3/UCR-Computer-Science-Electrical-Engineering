;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Yu Guo
; Email: yguo139@ucr.edu
; 
; Assignment name: Assignment 3
; Lab section: 
; TA:Samuel Tapia
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

AND R2, R2, #0
ADD R2, R2, #4          ;use R2 as counter, every 4 digits follow one space

AND R3, R3, #0
ADD R3, R3, #8
ADD R3, R3, #8          ;16 bits in total, so add 8 twice.

LOOP
    AND R1, R1, R1      ;so the next BR will operate based on R1   
    BRn #2
    LD R0, number_0
    OUT
    BRp #2
    LD R0, number_1
    OUT
    
    ADD R3, R3, #-1
    BRz #5
    ADD R2, R2, #-1
    BRp #3              ;if r2 is pisitive, skip the next 3 steps
    LD R0, Space        
    OUT                 ;print space
    ADD R2, R2, #4      ;reset r2 to 4
    
    ADD R1, R1, R1      ;shift left
    AND R3, R3, R3
BRp LOOP               ;keep looping when R3 is not 0
    
LD R0, newLine
OUT


HALT
;---------------	
;Data
;---------------
Value_ptr	.FILL xCA01	; The address where value to be displayed is stored
newLine     .FILL x0A

number_0    .FILL x30
number_1    .FILL x31
Space       .STRINGZ    " "

.END

.ORIG xCA01					; Remote data
Value .FILL xABCD			; <----!!!NUMBER TO BE DISPLAYED AS BINARY!!! Note: label is redundant.
;---------------	
;END of PROGRAM
;---------------	
.END
