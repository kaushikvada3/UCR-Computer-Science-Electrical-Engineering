;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Lab: lab 4, ex 3
; Lab section: 024  
; TA: Karan and Cody
;=================================================

; ==========================================================
; This is the test harness, or the 'main' of a C++ function
; ==========================================================
.ORIG x3000

; Main Program (Test Harness) Instructions:
LD R1, ARRAY_START          ; Load the address of the array into R1
LD R6, CONVERT_ASCII        ; R6 stores the value that has the value that converst between number to ASCII
LD R5, SUB_FILL_ARRAY_3200  ; Load the subroutine address into R5
JSRR R5                     ; Call the subroutine 'function' using the JSRR command
LD R5, SUB_CONVERT_ARRAY_3400 ; Load the new subroutine address into R5
JSRR R5          
LD R5, SUB_PRINT_ARRAY_3600 
JSRR R5; Call the new subroutine 'function' using the JSRR command
LD R5, SUB_PRETTY_PRINT_ARRAY_3800
JSRR R5
LEA R0, COMPLETED_MSG       ; Load the address of the completed message into R0
PUTS                        ; Print out completed message
HALT                        ; Halt the program

; Local data
ARRAY_START            .FILL x3100 ;points to the array
SUB_FILL_ARRAY_3200    .FILL x3200
SUB_CONVERT_ARRAY_3400 .FILL x3400
SUB_PRINT_ARRAY_3600   .FILL x3500
SUB_PRETTY_PRINT_ARRAY_3800 .FILL x3800
CONVERT_ASCII          .FILL #48
COMPLETED_MSG          .STRINGZ "The array has values from 0 through 9."
.END

;=============================================================
; Area for the subroutine (it's like defining the C++ function)
;=============================================================
.ORIG x3200

; Initialize counter in R2
AND R2, R2, #0              ; Clear R2 to start with 0

; Loop to fill the array
FILL_ARRAY_LOOP:
    STR R2, R1, #0          ; Store the current value of R2 into the array at R1
    ADD R2, R2, #1          ; Increment R2 by 1
    ADD R1, R1, #1          ; Increment the array address in R1 by 1
    ADD R3, R2, #-10        ; Subtract 10 from R2 and store the result in R3
    BRn FILL_ARRAY_LOOP     ; If R2 has not reached 10, repeat the loop

RET                         ; Return from subroutine
.END


;=============================================================
; Subroutine to convert between number to ASCII
;=============================================================
.ORIG x3400

; R1 initially contains the address of the first element of the array
; R2 is the counter and should be set to 0 before entering the loop
AND R2, R2, #0    ; Clear R2 (set R2 to 0)

CONVERT_LOOP:
    LDR R3, R1, #0
    ADD R3, R3, R6    ; Convert the current counter value to ASCII ('0' to '9')
    STR R3, R1, #0     ; Store the ASCII value at the current array location pointed by R1
    ADD R1, R1, #1     ; Increment the array address to point to the next element
    ADD R2, R2, #1     ; Increment the counter
    ADD R4, R2, #-10   ; Subtract 10 from the counter to set the condition for loop exit
    BRn CONVERT_LOOP   ; If R4 is negative (R2 < 10), loop again

RET                    ; Return from the subroutine

.END


;=============================================================
; Subroutine to print the array elements as ASCII characters
;=============================================================
.ORIG x3500

; R1 initially contains the address of the first element of the array
; R2 will be used as the counter, initialize it to 0
AND R2, R2, #0               ; Clear R2 to start with 0

PRINT_LOOP:
    LDR R0, R1, #0           ; Load the number from the array at R1 into R0
    ; ADD R0, R0, R6           ; Convert the number to its ASCII character ('0' to '9')
    OUT                      ; Output the character in R0
    ADD R1, R1, #1           ; Increment the array address to point to the next element
    ADD R2, R2, #1           ; Increment the counter
    ADD R3, R2, #-10         ; Subtract 10 from R2 to prepare the loop exit condition
    BRn PRINT_LOOP           ; If R3 is negative (meaning R2 < 10), repeat the loop

RET                          ; Return from subroutine
.END


;================================================
; Subroutine for Pretty Print
;================================================
.ORIG x3800

; Print the equal signs before the array
PRETTY_PRINT_START:
    LEA R0, EQUALS      ; Load the address of the equals sign string
    PUTS                ; Print the equals sign string
    
    ADD R2, R7, #0 ;back-up

; call the printing subroutine for array char printing
    JSR PRINT_LOOP ; Call the subroutine 'function' through the JSR

; print the equal signs after the array
    LEA R0, EQUALS      ; Load the address of the equals sign string
    PUTS                ; Print the equals sign string

   ADD R7, R2, #0   ;restore

RET                    ; Return from subroutine


EQUALS .STRINGZ "=====" ; String with 5 equal signs

.END


