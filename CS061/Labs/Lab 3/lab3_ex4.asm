;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
;
; Lab: Lab 3, Exercise 4 
; Lab section: 024
; TA: Karan and Cody
;=================================================

.ORIG x3000 ; Start of the program in local memory

LEA R3, REMOTE_ARRAY ; Address of  R3 is stored at R3
AND R4, R4, #0      ; Clear R4 to use as the character counter
LD R6, SENTINEL     ; Load the sentinel character 'a' into R6

; Input loop - reads characters until the sentinel character is inputted
INPUT_LOOP:
    GETC             ; Read a character from the keyboard to R0
    STR R0, R3, #0   ; Store the character into the remote array at index (R3)
    ADD R3, R3, #1   ; Move to the next memory location in the remote array
    ADD R4, R4, #1   ; Increment the character counter
    NOT R1, R0       ; Invert R0 to check against the sentinel
    ADD R1, R1, R6   ; If R1 + R6 == 0, then the sentinel has been read 
    BRz END_INPUT    ; Branch if the sentinel character is read
    BR INPUT_LOOP    ; Otherwise, continue loop

END_INPUT:
    STR R0, R3, #0   ; Store the sentinel character to mark the end of the input

; Prepare to output characters from the remote array
LEA R3, REMOTE_ARRAY ; Reload the address of the start of the remote array into R3

; Output loop - prints each character without newline after each character
OUTPUT_LOOP:
    LDR R0, R3, #0   ; Load the character from the remote array at index (R3) into R0
    ADD R1, R0, #0   ; Copy the character to R1 to check against the sentinel
    NOT R1, R1       ; Invert R1 to check against the sentinel
    ADD R1, R1, R6   ; If R1 + R6 == 0, then the sentinel has been read
    BRz OUTPUT_DONE  ; Branch if the sentinel character is loaded
    OUT              ; Output the character to the console
    ADD R3, R3, #1   ; Move to the next memory location in the remote array
    BR OUTPUT_LOOP   ; Otherwise, continue loop

OUTPUT_DONE:
    LD R0, NEWLINE   ; Load newline character into R0
    OUT              ; Output newline to console
    HALT             ; Stop the program

; Define the sentinel value and newline
SENTINEL .FILL x61     ; Sentinel 'a' charecter
NEWLINE .FILL x0A      ; Newline character
REMOTE_ARRAY .BLKW 100 ; Remote array of 100 elements

.END ; End of the program
