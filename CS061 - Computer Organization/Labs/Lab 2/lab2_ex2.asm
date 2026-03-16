;=================================================
; Name: Kaushik Vada
; Email:  vvada002@ucr.edu  
; 
; Lab: lab 2, ex 2
; Lab section: 024  
; TA: Karan and Cody
; 
;=================================================

    .ORIG x3000 ;starting me

    LDI R3, DEC_65_PTR      ; R3 <- Mem[ DEC_65 ]
    LDI R4, HEX_41_PTR      ; R4 <- Mem[ HEX_41 ]
    
    ;adding the values (their respective memory addresses by 1
    ADD R3, R3, #1
    ADD R4, R4, #1
    
    ;store the values back into the addresses using STI
    STI R3, DEC_65_PTR
    STI R4, HEX_41_PTR

    HALT
        
    DEC_65_PTR  .FILL x4000  ;These are now pointers pointing to memory location x4000
    HEX_41_PTR  .FILL x4001  ;These are now pointers pointing to memory location x4001
    

    .END               ; End of the program
    
    ;; Remote data
    .orig x4000
    NEW_DEC_65	.FILL #65
    NEW_HEX_41	.FILL x41
    .END

