;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 5, ex 1
; Lab section: 
; TA: Samuel Tapia 
; 
;=================================================

;====================================================================================================
; Main:
;   Test for PRINT_16BITS_NUMBERS_3200 subroutine
;====================================================================================================

                .ORIG x3000     ;Main starts here
                
;==============
; Instructions:
;==============
                LD R3, ARRAY_PTR        ;ptr to the array's first address
                LD R4, ARRAY_SIZE       ;used for loop count down
                LDR R5, R3, #10         ;starting number(1)

                ;--------------------------------------------------------------------------------------------------------input, fill 2^n into the array.
                LOOP_INPUT
                STR R5, R3, #0
                ADD R3, R3, #1      ; move the address to the next
                ADD R5, R5, R5      ; R5 <-- R5 + R5 (double the r5) because r5 start with 2^0. so the following r5 will be 2^1, 2^2, 2^3 ....
                ADD R4, R4, #-1     ; count down
                BRp LOOP_INPUT

                ;--------------------------------------------------------------------------------------------------------output
                LD R4, ARRAY_PTR        ;ptr to the array's first address
                LD R5, ARRAY_SIZE
                LD R6, PRINT_16BITS_NUMBERS
                
                LOOP_OUTPUT
                LDR R1, R4 #0
                jsrr r6
                ADD R4, R4 #1
                ADD R5, R5 #-1
                BRp LOOP_OUTPUT
                
                lea r0, completed_msg
                PUTS
                
                HALT
                
;-----------------------
;Local data (Main)
;-----------------------
ARRAY_PTR       .FILL x4000
ARRAY_SIZE      .FILL #10

PRINT_16BITS_NUMBERS    .FILL x3200     ;adress of subroutine
completed_msg           .stringz "program complete\n"
.END





;====================================================================================================
; Subroutine:: PRINT_16BITS_NUMBERS_3200
; Input(R1): the value whose 16-bits form will be printed
; 
; Postcondition: the subroutine will get the number from R1 and print it
;
; Returen value: This subroutine doens't have return value
;====================================================================================================
                .ORIG x3200     ;Subroutine starts here

; (1) back up r3 and r7
                st r3, backup_r3_3200
                st r7, backup_r7_3200

; (2) subroutine algorithm:

                AND R2, R2, #0 
                ADD R2, R2, #4          ;use R2 as counter, every 4 digits follow one space

                AND R3, R3, #0
                ADD R3, R3, #8
                ADD R3, R3, #8          ;16 bits in total, so add 8 twice.

                LOOP
                AND R1, R1, R1          ;so the next BR will operate based on R1   
                BRn #2
                LD R0, number_0
                OUT
                BRp #2
                LD R0, number_1
                OUT
    
                ADD R3, R3, #-1
                BRz #5
                ADD R2, R2, #-1
                BRp #3                  ;if r2 is pisitive, skip the next 3 steps
                LD R0, Space        
                OUT                     ;print space
                ADD R2, R2, #4          ;reset r2 to 4
    
                ADD R1, R1, R1          ;shift left
                AND R3, R3, R3
                BRp LOOP                ;keep looping when R3 is not 0
    
                LD R0, newLine
                OUT

; (3) restore backed up registers
                ld r3, backup_r3_3200
                ld r7, backup_r7_3200
                
; (4) return
                ret
;---------------	
;Data
;---------------
backup_r3_3200      .blkw #1
backup_r7_3200      .blkw #1

newLine             .FILL x0A
number_0            .FILL x30
number_1            .FILL x31
Space               .STRINGZ    " "
.END





;-----------------------
;Remote data
;-----------------------
.ORIG x4000
ARRAY           .BLKW #10
First_number    .FILL #1
.END



