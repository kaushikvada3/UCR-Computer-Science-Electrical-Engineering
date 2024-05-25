; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Assignment name: Assignment 5
; Lab section: 024
; TA: Karan and Cody
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
ld r6, stack_addr

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
                     
    LD R5, get_user_string_addr ;subroutine for getting the user input and to storing it
    JSRR R5 ;jumps directly to the subroutine

; find size of input string
; * put your code here

    LEA r1, user_string
    LD R5, strlen_addr ;subroutine that is used to find the length of the string
    JSRR R5 ;jumps to that subroutine

; call palindrome method
; * put your code here

    LEA R1, user_string         ;R1 is the address of the first letter in the string
    ADD R2, R1, R0              ;R2 is the last letter's address (still include 0(terminate value))
    AND R0, R0, R0              ;R0 is the length of the string
    BRz check_add               ;if the length of string is 0, then there is no need to reduce R2 by 1, so we move directly to check_add
        ADD R2, R2, #-1     
    
    check_add
        LD R5, palindrome_addr
        JSRR R5                     ;if r0 is 1, then it's palindrome. else it's not palindrome

; determine of stirng is a palindrome
; * put your code here
    ADD R1, R0, #0              ;store the result in r1 
    LEA R0, result_string
    PUTS
    
    AND R1, R1, R1              ;if r1 is 1, it's palindrome
    BRnp print_final_result

; print the result to the screen
; * put your code here
    print_final_result
    LEA R0, final_string
    PUTS

; decide whether or not to print "not"
; * put your code here
    LEA R0, not_string
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
final_string         .STRINGZ	"a palindrome\n"

; Reserve memory for user input string
user_string          .BLKW	  100

.END

;---------------------------------------------------------------------------------
; get_user_string - This subroutine gets the input from the user and stores it 
; parameter: R1 - The address of the user_prompt string 
; parameter: R2 - Taddress where the user string should be stored
;
; returns: nothing
;---------------------------------------------------------------------------------
.ORIG x3200
get_user_string
; Backup all used registers, R7 first, using proper stack discipline
    ST R7, backup_r7_3200  
    ST R6, backup_r6_3200
    ST R5, backup_r5_3200
    ST R3, backup_r3_3200
    ST R2, backup_r2_3200 
    
    
    
    

; Resture all used registers, R7 last, using proper stack discipline
.END

;---------------------------------------------------------------------------------
; strlen - DO NOT FORGET TO REPLACE THIS HEADER WITH THE PROPER HEADER
;---------------------------------------------------------------------------------
.ORIG x3300
strlen
; Backup all used registers, R7 first, using proper stack discipline

; Resture all used registers, R7 last, using proper stack discipline
.END

;---------------------------------------------------------------------------------
; palindrome - DO NOT FORGET TO REPLACE THIS HEADER WITH THE PROPER HEADER
;---------------------------------------------------------------------------------
.ORIG x3400
palindrome ; Hint, do not change this label and use for recursive alls
; Backup all used registers, R7 first, using proper stack discipline

; Resture all used registers, R7 last, using proper stack discipline
.END
