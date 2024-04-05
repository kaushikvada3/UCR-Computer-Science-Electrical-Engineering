;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Assignment name: Assignment 1
; Lab section: 024 
; TA: Karan and Cory
; 
; I hereby certify that I have not received assistance on this assignment,
; or used code, from ANY outside source other than the instruction team
; (apart from what was provided in the starter file).
;
;=========================================================================

;------------------------------------------
;           BUILD TABLE HERE
;------------------------------------------
;
; Register Values Table
;                R0    R1     R2    R3    R4    R5    R6      R7
;----------------------------------------------------------------
; Pre-loop:      0     6      12    0     0     0     0       0
; Iteration #01: 0     5      12    12    0     0     0       0
; Iteration #02: 0     4      12    24    0     0     0       0
; Iteration #03: 0     3      12    36    0     0     0       0
; Iteration #04: 0     2      12    48    0     0     0       0
; Iteration #05: 0     1      12    60    0     0     0       0
; Iteration #06: 0     0      12    72    0     0     0       0
; End of Program:0     32767  12    72    0     0     12286   0
;----------------------------------------------------------------



.ORIG x3000			; Program begins here
;-------------
;Instructions: CODE GOES HERE
;-------------
LD   R1, DEC_6    ; R1 <- 6
LD   R2, DEC_12   ; R2 <- 12
AND R3, R3, x0 ; "bitwise AND"
;LD   R3, DEC_0    ; could have done it this way

DO_WHILE  ADD   R3, R3, R2   ; R3 <- R3 + R2
          ADD   R1, R1, #-1  ; R1 <- R1 - 1
          BRp   DO_WHILE     ; if (R1 > 0) goto DO_WHILE
END_DO_WHILE


HALT
;---------------	
;Data (.FILL, .STRINGZ, .BLKW)
;---------------
DEC_6   .FILL #6   ; Put the value 6 into memory here
DEC_12  .FILL #12  ; Put the value 12 into memory here

;---------------	
; END of PROGRAM
;---------------	
.END


