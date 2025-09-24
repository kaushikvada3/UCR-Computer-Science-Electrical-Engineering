;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu 
; 
; Lab: lab 2, ex 4
; Lab section: 024  
; TA: Karan and Cody
; 
;=================================================

  .orig x3000
 
 
  LD R0, ASCII_CHAR
  LD R1, LOOP_COUNT
  
  
 DO_WHILE_LOOP
  OUT
  ADD R0,R0, #1
  ADD R1,R1 #-1
  BRp DO_WHILE_LOOP
 END_DO_WHILE_LOOP
 
 HALT
 
 ASCII_CHAR .FILL x61       ; Initial character 'a' (ASCII 97)
 LOOP_COUNT .FILL x1A       ; Count: 26
 
 .END
 
 