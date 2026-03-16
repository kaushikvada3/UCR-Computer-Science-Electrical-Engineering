;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Yu Guo
; Email: yguo139@ucr.edu
; 
; Assignment name: Assignment 2
; Lab section: 
; TA: Samuel Tapia
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

AND R1, R1, #0  ;use and to set the R1 to 0, then store the first digit to r1
AND R2, R2, #0  ;use and to set the R2 to 0, then store the first digit to r2

;-------------input the first number------------------------------------------------------------
GETC
OUT
ADD R1, R0, #0 ;save the number to r1 (ascii)

LD R0, newline      ; R0 = the value of ENDLINE
OUT

;;-------------input the second number------------------------------------------------------------
GETC
OUT
ADD R2, R0, #0 ;save the second number to r2 (ascii)

LD R0, newline
OUT

;-------------calculating and output part------------------------------------------------------------
AND R0, R0, #0  ;clear r0
ADD R0, R0, R1  ;store the number(at r2) to r0
OUT             ;print the number

LEA R0, Space    ;load " " to r0
PUTS             ;print " "
LEA R0, Minus    ;load "-" to r0
PUTS             ;print "-"
LEA R0, Space    ;load " " to r0
PUTS             ;print " "

AND R0, R0, #0  ;clear r0
ADD R0, R0, R2  ;store the number (at r1) to r0
OUT             ;print the number

LEA R0, Space   ;load " " to r0
PUTS            ;print " "
LEA R0, Equal   ;loar "=" to r0
PUTS            ;pring "="
LEA R0, Space   ;load " " to r0
PUTS            ;print " "

LD R3, hex_30
NOT R4, R3      ;1s complement of x30
ADD R4, R4, #1  ;2s complement of x30, now R4 hold the value -30

ADD R1, R1, R4
ADD R2, R2, R4

AND R5, R5, #0  ;clear r5
ADD R5, R5, R2  ;transfer r2 into r5
NOT R5, R5      ;1s complement
ADD R5, R5, #1  ;2s complement, now r5 hold the negative number of the second input
ADD R5, R5, R1  ; r5 + r1 => r5

Brzp #2  ;if R5 is positive, skip next 2 steps
    LEA R0, Minus    ;load "-" to r0
    PUTS             ;print "-"

AND R0, R0, #0  ; clear r0
ADD R0, R5, #0  ; pass r5 to r0

BRp #2  ;if ro is positive, skip the nextr 2 steps 
    ADD R0, R0, #-1     ;2s complement to 1s complement
    NOT R0, R0          

ADD R0, R0, R3  ; ADD 48 to change the number into ascii
OUT

LD R0, newline      ; R0 = the value of ENDLINE
OUT



;-------------------------------
;INSERT YOUR CODE here
;--------------------------------

HALT				; Stop execution of program
;------	
;Data
;------
; String to prompt user. Note: already includes terminating newline!
intro 	.STRINGZ	"ENTER two numbers (i.e '0'....'9')\n" 		; prompt string - use with LEA, followed by PUTS.

newline .FILL   x0A	; newline character - use with LD followed by OUT
hex_30  .FILL   x30 ;convert ASCII to Decimal and back (or #48)

Minus   .STRINGZ    "-"
Equal   .STRINGZ    "="
Space   .STRINGZ    " "


;---------------	
;END of PROGRAM
;---------------	

.END

