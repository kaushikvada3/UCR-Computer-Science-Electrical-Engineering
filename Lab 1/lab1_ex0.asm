;=================================================
; Name: Kaushik Vada    
; Email: vvada002@ucr.edu   
; 
; Lab: lab 1, ex 0
; Lab section: 024  
; TA: Karan Bhogal & Cody Kurpanek
; 
;=================================================

.ORIG x3000 ;this is the starting memory location (as mentioned in class)

;the code instructions go here
LEA R0, Message
PUTS


HALT ;this marks the end of the program (kind of like the last curly brace in a C++ program)
;local data (pseudo-op for hard coding data goes here) => Pseudo-Ops are directions that tell the assembler how to set up things
Message .STRINGZ "Hello World\n" 

.end   ; .end is like the “}” after main() in C++. 
       ; It means “no more code or data”


