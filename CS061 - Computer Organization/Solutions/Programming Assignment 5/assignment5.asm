; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Yu Guo
; Email: yguo139@ucr.edu
; 
; Assignment name: Assignment 5
; Lab section: 24
; TA: Samuel Tapia 
; 
; I hereby certify that I have not received assistance on this assignment,
; or used code, from ANY outside source other than the instruction team
; (apart from what was provided in the starter file).
;
;=================================================================================
; PUT ALL YOUR CODE AFTER THE main LABEL
;=================================================================================

;---------------------------------------------------------------------------------
;  Initialize program by setting stack pointer and calling main subroutine
;---------------------------------------------------------------------------------
.ORIG x3000

; initialize the stack
LD r6, stack_addr

; call main subroutine
lea r5, main
jsrr r5

;---------------------------------------------------------------------------------
; Main Subroutine
;---------------------------------------------------------------------------------
main
; get a string from the user
; * put your code here
    LEA R1, user_prompt 
    LEA R2, user_string
    
    LD R5, get_user_string_addr
    JSRR R5
                    
                    
; find size of input string
; * put your code here
    LEA r1, user_string
    LD R5, strlen_addr
    JSRR R5                     ;return the length of the string(stored in r0)
                    
                    
; call palindrome method
; * put your code here
    LEA R1, user_string         ;R1 is the address of the first letter in the string
    ADD R2, R1, R0              ;R2 is the last letter's address (still include 0(terminate value))
    AND R0, R0, R0              ;R0 is the length of the string
    BRz #1                      ;if the length of string is 0, then we don't need to -1 on r2 (-1 because we want to exclusive 0(terminate value))
        ADD R2, R2, #-1             ;R2 is the address of the last letter in the string
    
    LD R5, palindrome_addr
    JSRR R5                     ;if r0 is 1, then it's palindrome. else it's not palindrome
                    
; determine of stirng is a palindrome
; * put your code here
    ADD R1, R0, #0              ;store the result in r1 
    LEA R0, result_string
    PUTS
    
    AND R1, R1, R1              ;if r1 is 1, it's palindrome
    BRnp print_final_result
; decide whether or not to print "not"
; * put your code here
    LEA R0, not_string
    PUTS
                    
; print the result to the screen
; * put your code here
    print_final_result
    LEA R0, final_string
    PUTS
                    
                    
HALT

;---------------------------------------------------------------------------------
; Required labels/addresses
;---------------------------------------------------------------------------------

; Stack address ** DO NOT CHANGE **
stack_addr           .FILL    xFE00

; Addresses of subroutines, other than main
get_user_string_addr .FILL    x3200
strlen_addr          .FILL    x3300
palindrome_addr      .FILL	  x3400

; Reserve memory for strings in the progrtam
user_prompt          .STRINGZ "Enter a string: "
result_string        .STRINGZ "The string is "
not_string           .STRINGZ "not "
final_string         .STRINGZ "a palindrome\n"

; Reserve memory for user input string
user_string          .BLKW #100

.END

;---------------------------------------------------------------------------------
; get_user_string - get the user's input string and store it
; parameter: R1 - user_prompt string address
; parameter: R2 - address where the user string should be stored
;
; returns: nothing
;---------------------------------------------------------------------------------
.ORIG x3200
; Backup all used registers, R7 first, using proper stack discipline
    ST R2, backup_r2_3200    
    ST R3, backup_r3_3200      
    ST R5, backup_r5_3200
    ST R6, backup_r6_3200  
    ST R7, backup_r7_3200    
;--------------------------------------------------------------------
    AND R0, R0, #0
    ADD R0, R0, R1          
    PUTS                        ;if directly use ldr r0, r1, #0  then put will have an error(no idea why)
    
    LD R1, Hex_0A
    NOT R1, R1
    ADD R1, R1, #1              ;R1 = -12 (NEWLINE)
    
    INPUT_LOOP
        GETC
        OUT
        ADD R3, R0, #0          ;R3 = R0
        ADD R3, R3, R1
        BRnp #2                 ;if input is newline, end the loop
            STR R3, R2, #0      ;store 0. (string is end with 0)
            BR Input_Stop 
        STR R0, R2, #0          ;store input in the user_string block
        ADD R2, R2, #1          ;move the pointer to the next address
        BR INPUT_LOOP

    

    Input_Stop
;restore 
    LD R2, backup_r2_3200     
    LD R3, backup_r3_3200      
    LD R5, backup_r5_3200  
    LD R6, backup_r6_3200 
    LD R7, backup_r7_3200
;--------------------------------------------------------------------

    RET
;local data
backup_r2_3200      .blkw #1
backup_r3_3200      .blkw #1
backup_r5_3200      .blkw #1
backup_r6_3200      .blkw #1
backup_r7_3200      .blkw #1
Hex_0A              .FILL x0A
.END



;---------------------------------------------------------------------------------
; strlen - compute the length of a zero terminated string
;
; parameter: R1 - the address of a zero terminated string
;
; returns: r0 - the length of the string
;---------------------------------------------------------------------------------

.ORIG x3300
; Backup all used registers, R7 first, using proper stack discipline
    ST R2, backup_r2_3300    
    ST R3, backup_r3_3300      
    ST R5, backup_r5_3300 
    ST R6, backup_r6_3300 
    ST R7, backup_r7_3300    
;--------------------------------------------------------------------
    AND R0, R0, #0
    
    Count_Loop
        LDR R2, R1, #0      ;load the char from r1(string address) into r2
        BRz END_Count       ;once we detect 0(ascii code) in the string. End the loop
        ADD R0, R0, #1      ;add 1 to the length
        ADD R1, R1, #1      ;move the address to the next.
        BR Count_Loop
    
    END_Count
;restore            
    LD R2, backup_r2_3300     
    LD R3, backup_r3_3300      
    LD R5, backup_r5_3300 
    LD R6, backup_r6_3300
    LD R7, backup_r7_3300
;--------------------------------------------------------------------
                    
                    RET
;local data
backup_r2_3300      .blkw #1
backup_r3_3300      .blkw #1
backup_r5_3300      .blkw #1
backup_r6_3300      .blkw #1
backup_r7_3300      .blkw #1
.END



;---------------------------------------------------------------------------------
; palindrome - check if the string is parameter
; parameter: R1 - the address of the first letter in the string
;            R2 - the address of the last letter in the string
;            R6 - the address of stack    
; returns: R0 - if r0 is 1 then true, else false
;---------------------------------------------------------------------------------
.ORIG x3400
            
palindrome; Hint, do not change this label and use for recursive alls
; Backup all used registers, R7 first, using proper stack discipline
    ADD R6, R6, #-1
    STR R7, R6, #0          ;backup r7 in stack
    
    NOT R1, R1
    ADD R1, R1, #1          ;R1 = -R1 
    
    ADD R3, R1, R2          
    BRp #3                  ;if r1 < r2 , keep check the value
        AND R0, R0, #0      ;r1 and r2 are at the same address or r1 > r2
                            ;(r1 = r2 means the string length is odd, r1 > r2 means the string length is even, end the loop, return r0 (1)
        ADD R0, R0, #1      ;r0 == 1  
        BR DONE
    
    ADD R1, R1, #-1 
    NOT R1, R1              ;RESET R1 
    
    LDR R4, R1, #0
    LDR R5, R2, #0
    NOT R5, R5 
    ADD R5, R5, #1         
    
    ADD R3, R4, R5
    BRz #2                  ;if r4 - r5 != 0. means that the character values are different, and it's not palindrome
        AND R0, R0, #0      ;R0 == 0
        BR DONE 
        
    ADD R1, R1, #1          ;move r1 to the next address
    ADD R2, R2, #-1         ;move r2 to the front address
    JSR palindrome
; Resture all used registers, R7 last, using proper stack discipline
;--------------------------------------------------------------------
DONE        
    LDR R7, R6, #0          ;load r7 from the stack 
    ADD R6, R6, #1

    RET

;LOCAL DATA  
backup_r7_3400      .blkw #1

.END
