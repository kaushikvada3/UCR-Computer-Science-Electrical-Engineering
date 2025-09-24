;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Lab: lab 4, ex 1
; Lab section: 024  
; TA: Karan and Cody
;=================================================

; ==========================================================
; This is the test harness, or the 'main' of a C++ function
; ==========================================================
.ORIG x3000

; Main Program (Test Harness) Instructions:
LD R1, ARRAY_START         ; Load the address of the array into R1
LD R5, SUB_FILL_ARRAY_3200 ; Load the subroutine address into R5
JSRR R5                    ; Call the subroutine 'function' using the JSRR command
LEA R0, COMPLETED_MSG      ; Load the address of the completed message into R0
PUTS                       ; Print out completed message
HALT                       ; Halt the program

; Local data
ARRAY_START            .FILL x3100           ; Adjust as needed to point to your array's location
SUB_FILL_ARRAY_3200    .FILL x3200
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
