;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 5, ex 3
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
                

                
                AND R1, R1, #0
                ADD R1, R1, R2
                LD R6, PRINT_16BITS_NUMBERS_3200
                JSRR R6
                
                LEA R0, completed_msg
                PUTS
                
                HALT
                
;-----------------------
;Local data (Main)
;-----------------------
ascii_characters_EnterAndConvers    .FILL x3200     ;adress of subroutine
PRINT_16BITS_NUMBERS_3200           .FILL x3600


completed_msg           .stringz "\nprogram complete\n"

.END



;====================================================================================================
; Subroutine:: ascii_characters_EnterAndConvers_3200
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
                
                LD R5, Character_B
                NOT R5, R5
                ADD R5, R5, #1              ;Now r5 store the value -98, used for first input check
                
                First_Input
                    LEA R0, error_message_1
                    PUTS
                    
                    GETC
                    OUT
    
                    STR R0, R3, #0          ;Store the value
                    ADD R0, R0, R5
                    BRnp First_Input
                    
                ;---------------------------------  Input the rest numbers
                ADD R3, R3, #1          
                ADD R4, R4, #-1         
                
                LD R5, Decimal_48
                NOT R5, R5
                ADD R5, R5, #1              ;Now R5 store the value -48
                
                LD R6, Decimal_17
                
                LOOP                        ;get the user's input
                    GETC
                    OUT
                    STR R0, R3, #0          ;store the input into the array
                    
                    CHECK
                        ADD R0, R0, R5
                        BRz #7               ;if the sum of r0 and r5 is 0 means the input is 0(ascii ==> 48), jump out the loop
                        ADD R0, R0, #-1
                        BRz #5              ;if the sum of r0 and r5 and #-1 is 0 means that the input is 1 (ascii ==> 49), jump out the loop
                        ADD R0, R0, R6       ;if r0 is 0 after r6, then the input is space
                        BRz #2    
                            LEA R0, error_message_2
                            PUTS
                        BRnzp   LOOP
                    
                    ADD R3, R3, #1          ;move r3 to the next address(in the array)
                    ADD R4, R4, #-1
                    BRp LOOP
                    
                    
                    
                    
                ;------------------------------------------------------------------------------ Converse part 
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
;Local Data
;---------------
backup_r3_3200      .blkw #1
backup_r7_3200      .blkw #1

error_message_1     .stringz "\n The first input have to be lowercase b! \n"
error_message_2     .stringz "\n You can only input number 1 or 0, please re-enter the number: \n"

Character_B         .FILL #98
Decimal_48          .FILL #48
Decimal_17          .FILL #17           ;in the input part, i make the input -48 then -1 to check the input. if the input is space
                                        ;which ascii code is 32, then 32-48-1 = -17. after I check 1&0, then I sum the number with 17, if the result is 0, then don't 
                                        ;print error message!
                                        
ArrayPtr_1          .FILL x4000
Array_1_Size        .FILL #17                                        
.END



;====================================================================================================
; Subroutine:: PRINT_16BITS_NUMBERS_3600
; Input(R1): the value whose 16-bits form will be printed
; 
; Postcondition: the subroutine will get the number from R1 and print it
;
; Returen value: This subroutine doens't have return value
;====================================================================================================
                .ORIG x3600     ;Subroutine starts here

; (1) back up r2, r3 and r7
                st r2, backup_r2_3200_1
                st r3, backup_r3_3200_1
                st r7, backup_r7_3200_1

; (2) subroutine algorithm:
                LEA R0, Output_msg
                PUTS

                AND R2, R2, #0 
                ADD R2, R2, #4          ;use R2 as counter, every 4 digits follow one space

                AND R3, R3, #0
                ADD R3, R3, #8
                ADD R3, R3, #8          ;16 bits in total, so add 8 twice.

                LOOP_2
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
                BRp LOOP_2                ;keep looping when R3 is not 0
    
                LD R0, newLine
                OUT

; (3) restore backed up registers
                ld r2, backup_r2_3200_1
                ld r3, backup_r3_3200_1
                ld r7, backup_r7_3200_1
                
; (4) return
                ret
;---------------	
;Data
;---------------
backup_r2_3200_1      .blkw #1
backup_r3_3200_1      .blkw #1
backup_r7_3200_1      .blkw #1

Output_msg              .stringz "\n \n The binary number you entered is: \n"
newLine             .FILL x0A
number_0            .FILL x30
number_1            .FILL x31
Space               .STRINGZ    " "
.END


;---------------	
;Remote Data
;---------------
.ORIG x4000

Array_1             .blkw #17

.END



