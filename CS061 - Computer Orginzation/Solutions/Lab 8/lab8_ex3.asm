;=================================================
; Name: Yu Guo 
; Email: yguo139@ucr.edu
; 
; Lab: lab 8, ex 3
; Lab section: 24
; TA: Samuel Tapia 
; 
;=================================================



.ORIG x3000
                LEA R1, user_string         ;R1 - the starting address the character array
                LD R6, SUB_GET_STRING_PTR   ;R6 - the address of Subroutine: SUB_GET_STRING
                JSRR R6
                
                LD R6, SUB_TO_UPPER_PTR
                JSRR R6

                LD R6, SUB_IS_PALINDROME_PTR
                JSRR R6                     ;R4 {1 if the string is a palindrome, 0 otherwise}
                
                LEA R0, PALINDROME_start
                PUTS
                
                AND R4, R4, R4
                BRz #5
                    LEA R0, user_string 
                    PUTS
                    LEA R0, PALINDROME_YES
                    PUTS
                    BR end_program
                    
                LEA R0, user_string 
                PUTS
                LEA R0, PALINDROME_NO
                PUTS
                
                end_program
HALT

;--------------
;local data
;--------------
user_string          .BLKW #100
PALINDROME_start     .STRINGZ "\nThe string:\""

PALINDROME_YES       .STRINGZ "\" IS a palindrome. \n"
PALINDROME_NO        .stringz "\" IS NOT a palindrome. \n"
;--------------
;subroutine address
;--------------
SUB_GET_STRING_PTR      .FILL x3400
SUB_TO_UPPER_PTR      .FILL x3600
SUB_IS_PALINDROME_PTR   .FILL x3800
.END


.ORIG x3400
;----------------------------------------------------------------------------------------------------------------
; Subroutine: SUB_GET_STRING
; Parameter (R1): The starting address of the character array
; Postcondition: The subroutine has prompted the user to input a string,
;               terminated by the [ENTER] key (the "sentinel"), and has stored
;               the received characters in an array of characters starting at (R1).
;               the array is NULL-terminated; the sentinel character is NOT stored.
; Return Value (R5): The number of non-sentinel characters read from the user.
;                    R1 contains the starting address of the array unchanged.
;-----------------------------------------------------------------------------------------------------------------
; Backup all used registers
                    ST R1, backup_r1_3400
                    ST R2, backup_r2_3400    
                    ST R3, backup_r3_3400     
                    ST R4, backup_r4_3400     
                    ST R6, backup_r6_3400  
                    ST R7, backup_r7_3400    
;--------------------------------------------------------------------
                    LEA R0, user_prompt
                    PUTS
                    
                    LD R6, Hex_0A
                    NOT R6, R6
                    ADD R6, R6, #1              ;R6 = -12 (NEWLINE)
                    
                    AND R5, R5, #0              ;set r5 = 0, r5 is used to count the input string length
                    
                    INPUT_LOOP
                        GETC
                        OUT
                        ADD R3, R0, #0          ;R3 = R0
                        ADD R3, R3, R6
                        BRnp #2                 ;if input is newline, end the loop
                            STR R3, R1, #0      ;store 0. (string is end with 0)
                            BR Input_Stop 
                        STR R0, R1, #0          ;store input in the user_string block
                        ADD R1, R1, #1          ;move the pointer to the next address
                        ADD R5, R5, #1          ; R5 += 1
                        BR INPUT_LOOP

                    
           
                    Input_Stop
;restore            
                    LD R1, backup_r1_3400
                    LD R2, backup_r2_3400     
                    LD R3, backup_r3_3400
                    LD R4, backup_r4_3400 
                    LD R6, backup_r6_3400 
                    LD R7, backup_r7_3400
;--------------------------------------------------------------------

                    RET
;local data
user_prompt         .stringz "Enter the string: \n"
backup_r1_3400      .blkw #1
backup_r2_3400      .blkw #1
backup_r3_3400      .blkw #1
backup_r4_3400      .blkw #1
backup_r6_3400      .blkw #1
backup_r7_3400      .blkw #1

Hex_0A              .FILL x0A       ;newline
.END



.ORIG x3600
;------------------------------------------------------------------------------------------------------------------
; Subroutine: SUB_TO_UPPER
; Parameter (R1): Starting address of a null-terminated string
; Postcondition: The subroutine has converted the string to upper-case in-place
; i.e. the upper-case string has replaced the original string
; No return value, no output (but R1 still contains the array address, unchanged).
;------------------------------------------------------------------------------------------------------------------
; Backup all used registers
                    ST R1, backup_r1_3600
                    ST R2, backup_r2_3600    
                    ST R3, backup_r3_3600     
                    ST R4, backup_r4_3600 
                    ST R5, backup_r5_3600
                    ST R6, backup_r6_3600  
                    ST R7, backup_r7_3600    
;--------------------------------------------------------------------
                    LD R3, convert_Code
                    
                Loop_upper
                    LDR R0, R1, #0
                    BRz end_upper               ;load r0 from r1, if r0 = x0000, end the loop
                    
                    AND R0, R0, R3              ;change R0 to upper case
                    STR R0, R1, #0              ;store r0 into r1
                    ADD R1, R1, #1              ;move r1 to the next address
                    BR Loop_upper
                        
                    
                    end_upper
;restore            
                    LD R1, backup_r1_3600
                    LD R2, backup_r2_3600     
                    LD R3, backup_r3_3600
                    LD R4, backup_r4_3600 
                    LD R5, backup_r5_3600
                    LD R6, backup_r6_3600 
                    LD R7, backup_r7_3600
;--------------------------------------------------------------------
                    RET
;local data
backup_r1_3600      .blkw #1
backup_r2_3600      .blkw #1
backup_r3_3600      .blkw #1
backup_r4_3600      .blkw #1
backup_r5_3600      .blkw #1
backup_r6_3600      .blkw #1
backup_r7_3600      .blkw #1

convert_Code        .FILL X005F
.END




.ORIG x3800
;------------------------------------------------------------------------------------------------------------------
; Subroutine: SUB_IS_PALINDROME
; Parameter (R1): The starting address of a null-terminated string
; Parameter (R5): The number of characters in the array.
; Postcondition: The subroutine has determined whether the string at (R1) is
; a palindrome or not, and returned a flag to that effect.
; Return Value: R4 {1 if the string is a palindrome, 0 otherwise}
;------------------------------------------------------------------------------------------------------------------
; Backup all used registers
                    ST R1, backup_r1_3800
                    ST R2, backup_r2_3800    
                    ST R3, backup_r3_3800     
                    ST R5, backup_r5_3800     
                    ST R6, backup_r6_3800  
                    ST R7, backup_r7_3800        
;--------------------------------------------------------------------
                    AND R2, R2, #0
                    ADD R2, R1, R5          
                    ADD R2, R2, #-1         ;R2 is the address of the last letter
                    
                PALINDROME_CHECK
                    NOT R1, R1
                    ADD R1, R1, #1          ;set the first characters' address as negative
                    
                    ADD R3, R1, R2          
                    BRp #3                  ;if r1 < r2 , keep check the value
                        AND R4, R4, #0      ;r1 and r2 are at the same address or r1 > r2
                                            ;(r1 = r2 means the string length is odd, r1 > r2 means the string length is even, end the loop, return r4 (1)
                        ADD R4, R4, #1      ;r4 == 1  
                        BR DONE
                    
                    ADD R1, R1, #-1
                    NOT R1, R1              ;RESET R1
                    
                    LDR R4, R1, #0
                    LDR R5, R2, #0
                    NOT R5, R5 
                    ADD R5, R5, #1         
            
                    ADD R3, R4, R5
                    BRz #2                  ;if r4 - r5 != 0. means that the character values are different, and it's not palindrome
                        AND R4, R4, #0      ;R4 == 0
                        BR DONE 
                
                    ADD R1, R1, #1          ;move r1 to the next address
                    ADD R2, R2, #-1         ;move r2 to the front address
                    BR PALINDROME_CHECK

                    DONE
;restore            
                    LD R1, backup_r1_3800
                    LD R2, backup_r2_3800    
                    LD R3, backup_r3_3800     
                    LD R5, backup_r5_3800     
                    LD R6, backup_r6_3800  
                    LD R7, backup_r7_3800        
;--------------------------------------------------------------------

                    RET
;local data
backup_r1_3800      .blkw #1
backup_r2_3800      .blkw #1
backup_r3_3800      .blkw #1
backup_r5_3800      .blkw #1
backup_r6_3800      .blkw #1
backup_r7_3800      .blkw #1
.END



