;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu   
; 
; Lab: lab 2. ex 1
; Lab section: 024
; TA: Karan and Cody
; 
;=================================================



    .ORIG x3000 ;starting me

    LD R3, DEC_65      ; R3 <- Mem[ DEC_65 ]
    LD R4, HEX_41      ; R4 <- Mem[ HEX_41 ]

    HALT
        
    DEC_65  .FILL #65  ;DEC_65 being assigned with decimal value 65 (binary value: b1000001 )
    HEX_41  .FILL x41  ;HEX_41 being assigned with hexadecimal value of 41 (Decimal Value: #65 and Binary Value: b1000001) 
    ;^They are the same value

    .END               ; End of the program
