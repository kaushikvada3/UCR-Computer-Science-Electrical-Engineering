;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 5, ex 2
; Lab section: 
; TA: Samuel Tapia 
; 
;=================================================


;====================================================================================================
; Main:
;   Test for  ascii_characters_EnterAndConvers subroutine
;====================================================================================================

                .ORIG x3000     ;Main starts here
                
;==============
; Instructions:
;==============

                LD R6, ascii_characters_EnterAndConvers
                JSRR R6
                
                LEA R0, completed_msg
                PUTS
                
                HALT
                
;-----------------------
;Local data (Main)
;-----------------------
ascii_characters_EnterAndConvers    .FILL x3200     ;adress of subroutine
completed_msg           .stringz "\nprogram complete\n"

.END



;====================================================================================================
; Subroutine:: ascii_characters_EnterAndConvers
; Input: No input needed
; 
; Postcondition: the subroutine will let users input 17 ascii characters and transform it into a single 
;                16-bit value.
;
; Returen value(R2): The single 16-bit value will be stored in R2
;====================================================================================================
                .ORIG x3200     ;Subroutine starts here

; (1) back up r3 and r7
                st r3, backup_r3_3200
                st r7, backup_r7_3200

; (2) subroutine algorithm:
                
                LD R3, ArrayPtr_1
                LD R4, Array_1_Size
                
                LOOP                        ;get the user's input
                    GETC
                    OUT
                    
                    STR R0, R3, #0          ;store the input into the array
                    
                    ADD R3, R3, #1          ;move r3 to the next address(in the array)
                    ADD R4, R4, #-1
                    BRp LOOP
                ;------------------------------------------------------------------------------
                LD R2, Decimal_48 
                NOT R2, R2
                ADD R2, R2, #1              ;temporary store -48 in R2
                
                LD R3, ArrayPtr_1
                ADD R3, R3, #1              ;Because the first character in the array is 'b'
                LD R4, Array_1_Size
                ADD R4, R4, #-1             ;16 numbers in total
                
                AND R5, R5, #0              ;The calculation process is stored in R5
                
                Converse                    ;converse the number
                    LDR R1, R3, #0
                    ADD R1, R1, R2          
                    
                    BRz #2                  ;if the number is 1, r5*2 + 1
                        ADD R5, R5, R5
                        ADD R5, R5, #1
                    BRp #1                  ;if the number is 0, r5*2
                        ADD R5, R5, R5
                        
                    ADD R3, R3, #1          ;move r3 to the next address
                    ADD R4, R4, #-1         ;countdown
                    BRp Converse
                    
                AND R2, R2, #0
                ADD R2, R5, #0              ;move the value to r2
                    
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

Decimal_48          .FILL #48

ArrayPtr_1          .FILL x4000
Array_1_Size        .FILL 17
.END

.ORIG x4000
Array_1             .blkw #17
.END