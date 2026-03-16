;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 2. ex 4
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================


.ORIG x3000
;----------------------
;Program code
;----------------------

LD R0, Local_data_x61
LD R1, Local_data_x1A


DO_WHILE OUT
         ADD R0, R0, #1
         ADD R1, R1, #-1
         BRp DO_WHILE       ;if R1 is positive, goto DO_WHILE

HALT

    
;----------------------
;Local Data
;----------------------

Local_data_x61  .FILL x61
Local_data_x1A  .FILL x1A

.END
