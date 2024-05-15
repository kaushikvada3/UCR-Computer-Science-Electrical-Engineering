;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik Vadab
; Email: vvada002@ucr.edu
; 
; Assignment name: Assignment 2
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

;----------------------------------------------
;output prompt
;----------------------------------------------	
LEA R0, intro			; get starting address of prompt string
PUTS			    	; Invokes BIOS routine to output string

;-------------------------------
;INSERT YOUR CODE here
;--------------------------------
;need to initialize a register to store the number to convert ASCII to numbers
LD R5, ASCII_TO_NUMBER
LD R6, NUMBER_TO_ASCII


; Need to read in the FIRST DIGIT from the console and store it

; Read in the first digit
GETC                   ; Get the first character (digit)
OUT                    ; Echo the digit
add r6, r0, #0
ADD R2, R0, R5         ; Convert from ASCII to integer

LD R0, newLine
OUT

; Read in the second digit
GETC                   ; Get the second character (digit)
OUT                    ; Echo the digit
add r7, r0, #0
ADD R3, R0, R5         ; Convert from ASCII to integer


LD R0, newLine
OUT

; time to perform subtraction (Num1 - Num2)

add r0, r6, #0
out

lea r0, minus
puts

add r0, r7, #0
out

lea r0, equalSign
puts

LD R6, NUMBER_TO_ASCII
;take the 2's complement of 2nd digit
NOT R3, R3 
ADD R3, R3, #1
ADD R4, R2, R3 ;Subtract R4 = R2 + (-R3)


; check to see if the result is negative
BRn PERFORM_NEGATIVE ;if it does end up negative, then it will jump to PERFORM_NEGATIVE (sort of like a function in C++)

; If result is positive, display it
; LEA R0, operation      ; Load the operation message
; PUTS                   ; Output the operation message
ADD R0, R4, R6         ; Convert result back to ASCII
OUT                    ; Output the result
BRnzp SKIP           ; Go to SKIP

; if the result is negative
PERFORM_NEGATIVE
LEA R0, negVal
PUTS
NOT R4, R4
ADD R4, R4, #1
ADD R0, R4, R6       ; Convert the magnitude back to ASCII
OUT                  ; Print the result


SKIP

LD R0, newLine
OUT
HALT				; Stop execution of program
;------	
;Data
;------
; String to prompt user. Note: already includes terminating newline!
ASCII_TO_NUMBER  .FILL #-48
NUMBER_TO_ASCII  .FILL #48
intro 	.STRINGZ	"ENTER two numbers (i.e '0'....'9')\n" 		; prompt string - use with LEA, followed by PUTS.
newline .FILL x0A	; newline character - use with LD followed by OUT
operation        .STRINGZ "-"
negVal           .STRINGZ "-"
minus           .STRINGZ " - "
equalSign        .STRINGZ " = "
;---------------	
; END of PROGRAM
;---------------	
.END