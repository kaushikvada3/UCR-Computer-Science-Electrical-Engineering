;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 2. ex 2
; Lab section: 024
; TA: Samuel Tapia
; 
;=================================================

.ORIG x3000
;----------------------
;Program code
;----------------------

LDI R3, DEC_65_PTR
LDI R4, HEX_41_PTR

ADD R3, R3, #1
STI R3, DEC_65_PTR

ADD R4, R4, #1
STI R4, HEX_41_PTR

HALT
    
;----------------------
;Local Data
;----------------------

DEC_65_PTR  .FILL   X4000
HEX_41_PTR  .FILL   X4001

.END

.ORIG   X4000               ;Remote data located at address x4000
;----------------------
; Remote Data
;----------------------
DEC_65  .FILL   #65
HEX_41  .FILL   x41
    
.END