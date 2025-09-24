;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 2. ex 1
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
;;;;;;;;;;;
;Program code
;;;;;;;;;;;

    LD R3, DEC_65    ;load DEC_51 to register r3
    
    LD R4, HEX_41    ;load HEX_41 to register r4

    HALT

;;;;;;;;;;;
;Local Data
;;;;;;;;;;;

    DEC_65  .FILL   #65 ;represent A in ascii table
    HEX_41  .FILL   x41 ;represent A in ascii table
    
    
.END