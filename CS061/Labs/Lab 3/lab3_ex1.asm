;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Lab: Lab 3, Exercise 1
; Lab section: 024
; TA: Karan and Cody
;=================================================

.ORIG x3000 ; Start of the program in local memory

; Load the base address of the remote data block into R5
LDI R5, DATA_PTR ; R5 <- Mem[Mem[DATA_PTR]]

; Access the first data value using the base pointer
LDR R3, R5, #0 ; R3 <- Mem[R5 + 0] (points to x4000)

; Access the second data value using address arithmetic (offset +1)
LDR R4, R5, #1 ; R4 <- Mem[R5 + 1] (points to x4001)

; Modify the data items
ADD R3, R3, #1 ; Increment the value at x4000
ADD R4, R4, #1 ; Increment the value at x4001

; Store the modified values back to their respective addresses
STR R3, R5, #0 ; Mem[R5 + 0] <- R3 (store back at x4000)
STR R4, R5, #1 ; Mem[R5 + 1] <- R4 (store back at x4001)

HALT ; Stop the program

; Pointer to the start of the remote data block
DATA_PTR .FILL x4000 ; Pointer to memory location x4000

.END ; End of the program

;; Remote data
.ORIG x4000
NEW_DEC_65 .FILL #65 ; Initial value at x4000
NEW_HEX_41 .FILL x41 ; Initial value at x4001
.END