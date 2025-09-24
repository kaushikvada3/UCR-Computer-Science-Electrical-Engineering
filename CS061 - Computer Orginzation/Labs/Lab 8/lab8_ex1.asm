;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu 
; 
; Lab: lab 8, ex 1
; Lab section: 024 
; TA: Karan and Cody
; 
;=================================================

; test harness
.orig x3000
    LD R6, LOAD_FILL_VALUE_3200_ptr
    JSRR R6                         ;return r1

    ADD R1, R1, #1
    LD R6, OUTPUT_AS_DECIMAL_3400_ptr
    JSRR R6
    
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:


;--------------------
;subroutines_address
;--------------------
LOAD_FILL_VALUE_3200_ptr  .FILL    x3200
OUTPUT_AS_DECIMAL_3400_ptr  .FILL    x3400

;===============================================================================================
.end


; subroutines:

;------------------------------------------------------------------------------------------
; Subroutine: LOAD_FILL_VALUE_3200 
; Parameter (R):  NONE
; Return Value:   R1 (The hardcoded value)
;------------------------------------------------------------------------------------------
.orig x3200
;BACK UP:
            ST R2, backup_r2_3200    
            ST R3, backup_r3_3200     
            ST R4, backup_r4_3200
            ST R5, backup_r5_3200
            ST R6, backup_r6_3200  
            ST R7, backup_r7_3200  
				 
		    LD R1, Num  ;the hardcoded value
				 
;restore            
            LD R2, backup_r2_3200     
            LD R3, backup_r3_3200
            LD R4, backup_r4_3200 
            LD R5, backup_r5_3200
            LD R6, backup_r6_3200 
            LD R7, backup_r7_3200	 
ret
;-----------------------------------------------------------------------------------------------
; SUB_LOAD_VALUE local data
backup_r2_3200  .BLKW #1
backup_r3_3200  .BLKW #1
backup_r4_3200  .BLKW #1
backup_r5_3200  .BLKW #1
backup_r6_3200  .BLKW #1
backup_r7_3200  .BLKW #1

Num             .FILL #-6
.end






;===============================================================================================


;------------------------------------------------------------------------------------------
; Subroutine: OUTPUT_AS_DECIMAL_3400 
; Parameter (R): R1 (BINARY NUMBER)
; Return Value: NONE
;------------------------------------------------------------------------------------------
.orig x3400

; Backup the registers to preserve their values
ST R1, backup_r1_3400            ; Store R1
ST R2, backup_r2_3400            ; Store R2
ST R3, backup_r3_3400            ; Store R3
ST R4, backup_r4_3400            ; Store R4
ST R5, backup_r5_3400            ; Store R5
ST R6, backup_r6_3400            ; Store R6
ST R7, backup_r7_3400            ; Store R7

; Check if the number in R1 is negative
AND R1, R1, R1
brzp #4                          ; Branch if R1 is non-negative
    ADD R1, R1, #-1              ; Prepare R1 for two's complement (invert and add 1)
    NOT R1, R1                   ; Convert R1 to its positive equivalent
    LD R0, hex_2D                ; Load ASCII code for '-' (minus sign)
    OUT                          ; Output the minus sign

; Initialize constants for the 10000s place
LD R2, num_10000s                ; Load the negative of 10000 into R2
LD R3, num_0                     ; Load ASCII '0' into R3
AND R0, R0, #0                   ; Clear R0 to use as a counter

; Loop to extract and print the 10000s place digit
_10000s_place_loop
    ADD R1, R1, R2               ; Subtract 10000 from R1
    BRzp #9                      ; Branch if R1 is non-negative
        ADD R2, R2, #-1          ; Restore the original value of R1
        NOT R2, R2               
        ADD R1, R1, R2           ; Restore the original value of R1
        AND R0, R0, R0           ; Check the counter value
        BRz #2                   ; Skip printing if counter is zero (suppress leading zero)
            ADD R0, R0, R3       ; Convert the digit count to ASCII
            OUT                  ; Output the digit
        ADD R0, R0, R3           ; Adjust R0 for the next place value
        BR _1000s_place_loop_pre ; Branch to setup for the 1000s place
    ADD R0, R0, #1               ; Increment the digit counter
    BR _10000s_place_loop        ; Continue the loop for 10000s place

; Prepare for extracting the 1000s place
_1000s_place_loop_pre
LD R2, num_1000s                 ; Load the negative of 1000 into R2
NOT R3, R3                       ; Invert R3 to handle the subtraction
ADD R3, R3, #1                   ; Adjust R3 for use in digit calculation
ADD R0, R0, R3                   ; Adjust the counter by subtracting ASCII '0'
BRp #2                           ; Check if we need to output a zero for this place
    ADD R6, R0, #1               ; Set a flag if the previous place's digit was zero
    BR #2
AND R6, R6, #0                   ; Reset flag R6
AND R0, R0, #0                   ; Reset the counter

; Loop to extract and print the 1000s place digit
_1000s_place_loop
    ADD R1, R1, R2               ; Subtract 1000 from R1
    BRzp #17                     ; Continue in loop if R1 is non-negative
        ADD R2, R2, #-1          ; Restore R2 to negative 1000
        NOT R2, R2               
        ADD R1, R1, R2           ; Restore the original R1 value
        AND R0, R0, R0           ; Check the counter
        BRz #4                   ; Skip output if counter is zero
            LD R3, num_0         ; Reset R3 to ASCII '0'
            ADD R0, R0, R3
            OUT                  ; Output the digit
            BR _100s_place_loop_pre
        AND R6, R6, R6
        Brp #3
            LD R3, num_0         ; Prepare to output digit if leading zeros not needed
            ADD R0, R0, R3
            OUT
        LD R3, num_0
        ADD R0, R0, R3
        BR _100s_place_loop_pre
    ADD R0, R0, #1
    BR _1000s_place_loop

; Prepare for extracting the 100s place
_100s_place_loop_pre
LD R2, num_100s                  ; Load the negative of 100 into R2
NOT R3, R3                       ; Invert R3 to prepare for subtraction
ADD R3, R3, #1                   ; Adjust R3 to convert counter to ASCII
ADD R0, R0, R3                   ; Adjust the counter by subtracting ASCII '0'
BRp #2                           ; Check if we need to output a zero for this place
    ADD R6, R0, #1               ; Set a flag if previous places were zero
    BR #2
AND R0, R0, #0                   ; Reset the counter
AND R6, R6, #0                   ; Reset flag R6

; Loop to extract and print the 100s place digit
_100s_place_loop
    ADD R1, R1, R2               ; Subtract 100 from R1
    BRzp #17                     ; Continue in loop if R1 is non-negative
        ADD R2, R2, #-1          ; Restore R2 to negative 100
        NOT R2, R2               
        ADD R1, R1, R2           ; Restore the original R1 value
        AND R0, R0, R0           ; Check the counter
        BRz #4                   ; Skip output if counter is zero
            LD R3, num_0         ; Reset R3 to ASCII '0'
            ADD R0, R0, R3
            OUT                  ; Output the digit
            BR _10s_place_loop_pre
        AND R6, R6, R6
        Brp #3
            LD R3, num_0         ; Prepare to output digit if leading zeros not needed
            ADD R0, R0, R3
            OUT
        LD R3, num_0
        ADD R0, R0, R3
        BR _10s_place_loop_pre
    ADD R0, R0, #1
    BR _100s_place_loop

; Prepare for extracting the 10s place
_10s_place_loop_pre
LD R2, num_10s                   ; Load the negative of 10 into R2
LD R3, num_0                     ; Reset R3 to ASCII '0'
NOT R3, R3                       ; Invert R3 to prepare for subtraction
ADD R3, R3, #1                   ; Adjust R3 to convert counter to ASCII
ADD R0, R0, R3                   ; Adjust the counter by subtracting ASCII '0'
BRp #2                           ; Check if we need to output a zero for this place
    ADD R6, R0, #1               ; Set a flag if previous places were zero
    BR #2
AND R0, R0, #0                   ; Reset the counter
AND R6, R6, #0                   ; Reset flag R6

; Loop to extract and print the 10s place digit
_10s_place_loop
    ADD R1, R1, R2               ; Subtract 10 from R1
    BRzp #17                     ; Continue in loop if R1 is non-negative
        ADD R2, R2, #-1          ; Restore R2 to negative 10
        NOT R2, R2               
        ADD R1, R1, R2           ; Restore the original R1 value
        AND R0, R0, R0           ; Check the counter
        BRz #4                   ; Skip output if counter is zero
            LD R3, num_0         ; Reset R3 to ASCII '0'
            ADD R0, R0, R3
            OUT                  ; Output the digit
            BR _1s_place_loop_pre
        AND R6, R6, R6
        Brp #3
            LD R3, num_0         ; Prepare to output digit if leading zeros not needed
            ADD R0, R0, R3
            OUT
        LD R3, num_0
        ADD R0, R0, R3
        BR _1s_place_loop_pre
    ADD R0, R0, #1
    BR _10s_place_loop

; Prepare for extracting and printing the 1s place (final digit)
_1s_place_loop_pre
LD R3, num_0                     ; Load ASCII '0' into R3
ADD R0, R1, R3                   ; Convert the final digit to ASCII
OUT                              ; Output the final digit

; Restore all registers from backup before returning
LD R1, backup_r1_3400
LD R2, backup_r2_3400
LD R3, backup_r3_3400
LD R4, backup_r4_3400
LD R5, backup_r5_3400
LD R6, backup_r6_3400
LD R7, backup_r7_3400

; Return from the subroutine
ret

;-----------------------------------------------------------------------------------------------
; SUB_PRINT_DECIMAL local data
backup_r1_3400  .BLKW #1
backup_r2_3400  .BLKW #1
backup_r3_3400  .BLKW #1
backup_r4_3400  .BLKW #1
backup_r5_3400  .BLKW #1
backup_r6_3400  .BLKW #1
backup_r7_3400  .BLKW #1

hex_2D          .FILL x2d   ;"-"
num_0           .FILL #48
num_10000s      .FILL #-10000
num_1000s       .FILL #-1000
num_100s        .FILL #-100
num_10s         .FILL #-10
num_1s          .FILL #-1
.end

