;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 8, ex 1
; Lab section: 24
; TA: Samuel Tapia  
; 
;=================================================

.ORIG x3000
                LEA R1, user_string         ;R1 - the starting address the character array
                LD R6, SUB_GET_STRING_PTR   ;R6 - the address of Subroutine: SUB_GET_STRING
                
                JSRR R6

HALT

;--------------
;local data
;--------------
user_string          .BLKW #100


;--------------
;subroutine address
;--------------
SUB_GET_STRING_PTR  .FILL x3400

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
; Backup all used registers, R7 first, using proper stack discipline
                    ST R1, backup_r1_3400
                    ST R2, backup_r2_3400    
                    ST R3, backup_r3_3400      
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
                    LD R6, backup_r6_3400 
                    LD R7, backup_r7_3400
;--------------------------------------------------------------------

                    RET
;local data
user_prompt         .stringz "Enter the string: \n"
backup_r1_3400      .blkw #1
backup_r2_3400      .blkw #1
backup_r3_3400      .blkw #1
backup_r6_3400      .blkw #1
backup_r7_3400      .blkw #1

Hex_0A              .FILL x0A       ;newline
.END

