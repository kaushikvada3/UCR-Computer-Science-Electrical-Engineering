.ORIG x3000			; Program begins here

LD R6, Value_ptr		; R6 <-- pointer to value to be displayed as binary
LDR R1, R6, #0			; R1 <-- value to be displayed as binary 

; Initialize loop counter in R5 to 16
AND R5, R5, #0
LD R5, SIXTEEN

; Initialize bit counter for spacing in R4
AND R4, R4, #0

PRINT_LOOP:
    AND R3, R1, #0x8000 ; Isolate the MSB of R1
    BRz PRINT_ZERO      ; If MSB is 0, jump to PRINT_ZERO

    ; Output '1'
    LD R0, CHAR_ONE
    OUT
    BRnzp NEXT_BIT

PRINT_ZERO:
    ; Output '0'
    LD R0, CHAR_ZERO
    OUT

NEXT_BIT:
    ; Increment bit counter and check if it's time to add a space
    ADD R4, R4, #1
    AND R2, R4, #3      ; R2 = R4 mod 4 (Check if R4 is 4)
    BRz PRINT_SPACE
    BR SHIFT_BIT

PRINT_SPACE:
    LD R0, CHAR_SPACE
    OUT
    AND R4, R4, #0      ; Reset bit counter after printing space

SHIFT_BIT:
    ADD R1, R1, R1       ; Left shift R1
    ADD R5, R5, #-1     ; Decrement the loop counter
    BRp PRINT_LOOP      ; Continue if not done with all bits

HALT
; Data section
Value_ptr  .FILL xCA01
CHAR_ONE   .FILL x0031  ; ASCII for '1'
CHAR_ZERO  .FILL x0030  ; ASCII for '0'
CHAR_SPACE .FILL x0020  ; ASCII for space
SIXTEEN .FILL #16

.END

.ORIG xCA01
Value .FILL xABCD       ; NUMBER TO BE DISPLAYED AS BINARY
.END
