;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Lab: lab 2, ex 3
; Lab section: 024  
; TA: Karan and Cody
; 
;=================================================


    .ORIG x3000 ;starting me

    LDI R3, DEC_65_PTR      ; R3 <- Mem[Mem[ DEC_65_PTR ]]
    LDI R4, HEX_41_PTR      ; R4 <- Mem[Mem[ HEX_41_PTR ]]
    
    ;adding the values (their respective memory addresses by 1
    ADD R3, R3, #1
    ADD R4, R4, #1
    
    ;store the values back into the addresses using STI
    STI R3, DEC_65_PTR    ;Mem[Mem[DEC_65_PTR]] <- R3 (value of R3 is being storeed into the memory address of DEC_65_PTR)
    STI R4, HEX_41_PTR    ;Mem[Mem[HEX_41_PTR]] <- R4 (value of R4 is being storeed into the memory address of HEX_41_PTR)
    
    LD R5, DEC_65_PTR   ;R5 <- x4000
    LD R6, HEX_41_PTR   ;R5 <- x4001
    
    ;LDR ("pick the item "blank" items to the right")
    LDR R3, R5, #0     ; R3 <- Mem[R5 + 0] (R5 points to x4000)
    LDR R4, R6, #0     ; R4 <- Mem[R6 + 0] (R6 points to x4001)
    
    ADD R3, R3, #1     ; Increment the value in R3 by 1
    ADD R4, R4, #1     ; Increment the value in R4 by 1

    ;STR
    STR R3, R5, #0     ; Mem[R5 + 0] <- R3 (Store back at x4000)
    STR R4, R6, #0     ; Mem[R6 + 0] <- R4 (Store back at x4001)

    HALT
        
    DEC_65_PTR  .FILL x4000  ;These are now pointers pointing to memory location x4000
    HEX_41_PTR  .FILL x4001  ;These are now pointers pointing to memory location x4001
    

    .END               ; End of the program
    
    ;; Remote data
    .orig x4000
    NEW_DEC_65	.FILL #65
    NEW_HEX_41	.FILL x41
    .END