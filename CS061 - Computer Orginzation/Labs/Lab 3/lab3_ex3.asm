;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
;
; Lab: Lab 3, Exercise 2
; Lab section: 024
; TA: Karan and Cody
;=================================================

.ORIG x3000 ; Start of the program in local memory

; Initialize the counter
AND R2, R2, #0 ; Clear R2 to use as counter
LEA R3, ARRAY ; Load the address of the start of the array into R3

; Loop to input characters
INPUT_LOOP:
    ADD R2, R2, #1 ; Increment counter
    GETC      ; Get inputs from the user
    STR R0, R3, #0 ; Store the character from R0 into ARRAY at index (R3)
    ADD R3, R3, #1 ; Move to the next memory location in the array
    ADD R1, R2, #-10 ; Check if 10 characters have been input
    BRnp INPUT_LOOP ; If not, loop again
    
; Reset the counter for output loop
AND R2, R2, #0 ; Clear R2 to use as counter for output loop
LEA R3, ARRAY ; Reload the address of the start of the array into R3

; Loop to output characters
OUTPUT_LOOP:
    LDR R0, R3, #0 ; Load the character from ARRAY at index (R3) into R0
    OUT            ; Output the 'character' in R0
    LD R0, NEWLINE ; Load the newline character into R0
    OUT            ; Output the newline 'character'
    ADD R3, R3, #1 ; Move to the next memory location
    ADD R2, R2, #1 ; Increment counter
    ADD R1, R2, #-10 ; Check if 10 characters have been output
    BRnp OUTPUT_LOOP ; If not, loop again


; Program completes after filling array
HALT ; Stop the program

; Define the array space
NEWLINE .FILL x0A
ARRAY .BLKW 10 ; Array of 10 characters in the local data area

.END ; End of the program
