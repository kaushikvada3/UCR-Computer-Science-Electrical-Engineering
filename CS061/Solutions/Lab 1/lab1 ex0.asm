;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 1, ex 0
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
;;;;;;;;;;;
;Program code
;;;;;;;;;;;

    LEA R0, MSG_TO_PRINT
    PUTS        ;Pring string defined as MSG_TO_PRINT
    
    HALT

;;;;;;;;;;;
;local data
;;;;;;;;;;;

    MSG_TO_PRINT .STRINGZ "Hello World!\n"
    
.END