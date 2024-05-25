;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Kaushik Vada
; Email: vvada002@ucr.edu
; 
; Assignment name: Assignment 4
; Lab section: 024
; TA: Karan and Cody
; 
; I hereby certify that I have not received assistance on this assignment,
; or used code, from ANY outside source other than the instruction team
; (apart from what was provided in the starter file).
;
;=================================================================================
;THE BINARY REPRESENTATION OF THE USER-ENTERED DECIMAL NUMBER MUST BE STORED IN R4
;=================================================================================

.ORIG x3000		
;-------------
;Instructions
;-------------

; output intro prompt
Intro_Prompt
    LD R0, IntroPromptMessage
    PUTS
						
; Set up flags, counters, accumulators as needed
   LD R1, Arr_Pointer
   LD R2, Arr_size

   AND R4, R4, #0 ; this is to clear the register to store the final result

   AND R5, R5, #0
   AND R6, R6, #0

   LD R5, ASCII_NEG_SIGN ;holds the ASCII version of the 'negative' sign
   NOT R5, R5 ;flips all the bits so it does end up becoming a negative number
   ADD R5, R5, #1 ;adding 1 to the value to complete 2's complement procedure

   LD R6, NewSpace
   NOT R6, R6 ;flips all the bits
   ADD R6, R6, #1 ;the 2's complement is done to make sure when you are using comparing, and if it matches it will add to zero

   AND R3, R3, #0 ;R3 will be used to check the input

; Get first character, test for '\n', '+', '-', digit/non-digit 

   ;input procedures
      GETC
      OUT

      ADD R3, R0, R5 ;always reads it into R0
      BRz negativeSign
         ADD R3, R3, #2
         BRz positiveSign
					
; is very first character = '\n'? if so, just quit (no message)!
    AND R3, R0, #0
    ADD R3, R6, R0
    BRz ProgramHalt

    BRnp Greater_Than_Zero ;if its not one of the signs, then go onto check the digit
  

; is it = '+'? if so, ignore it, go get digits
positiveSign
   ADD R2, R2, #-1
     BRnp Processing_Step

; is it = '-'? if so, set neg flag, go get digits
negativeSign
   STR R0, R1, #0
   ADD R1, R1, #1
   ADD R2, R2, #-1
   BRnp Processing_Step
					
; is it < '0'? if so, it is not a digit	- o/p error message, start over
Greater_Than_Zero
  LD R5, Hex_zero_val       ; Load the ASCII value for '0' into R5
  NOT R5, R5            ; Perform 2's complement
  ADD R5, R5, #1       

  ADD R5, R0, R5        ; Add R0 (input character) and R5 (-48). This subtracts '0' from the input character.
  BRzp Greater_Than_nine; If the result is >= 0, branch to Greater_Than_nine
  
  LD R0, Error_Message  ; If the input character was less than '0', load the error message address into R0
  PUTS                  ; Output the error message string
  BRnzp Intro_Prompt    ; Branch to Intro_Prompt to restart the input prompt


; is it > '9'? if so, it is not a digit	- o/p error message, start over
Greater_Than_nine
    LD  R5, Hex_nine_val
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-57", use to check if the input is "9" or not
    
    ADD R3, R0, R5
    BRnz NumStore             ; if it's <'9' store the value
        LD R0, Error_Message
        PUTS 
        BRnzp Intro_Prompt

    NumStore
        STR R0, R1, #0
        ADD R4, R4, R0              ;store the input value into R4 (array)
        ADD R1, R1, #1              ;move on the next array address
        ADD R2, R2, #-1              

				
; if none of the above, first character is first numeric digit - convert it to number & store in target register!
    LD R5, Hex_zero_val
    NOT R5, R5
    ADD R5, R5, #1
    ADD R4, R4, R5 ;holds the value of the real input

Processing_Step
    AND R4, R4, R4  
					
; Now get remaining digits from user in a loop (max 5), testing each to see if it is a digit, and build up number in accumulator

Num_Input_Loop
        GETC
        OUT
        
        AND R3, R3, #0
        ADD R3, R6, R0
        BRz quit                   ;If the input symbol's ascii code is 10(newline) end the input loop
        
        LD  R5, Hex_zero_val
        NOT R5, R5
        ADD R5, R5, #1              ;R5 store "-48", use to check if the input is "0" or not

        ADD R3, R0, R5
        BRzp label1                      ;if >'0', go check if it's >9
        LD R0, Error_Message
        PUTS 
        BRnzp Intro_Prompt
        
        label1
        LD  R5, Hex_nine_val
        NOT R5, R5
        ADD R5, R5, #1              ;R5 store "-57", use to check if the input is "0" or not

        ADD R3, R0, R5
        BRnz label_store                      ;if it's <'9' store the value
        LD R0, Error_Message
        PUTS 
        BRnzp Intro_Prompt
        
        label_store
        STR R0, R1, #0              ;Store the input into the array
        
        LD  R5, Hex_zero_val
        NOT R5, R5
        ADD R5, R5, #1              ;R5 store "-48"
        ADD R0, R5, R0              ;R0 hold the real input number
        
        AND R3, R3, #0
        
        ;multiplies it by 10 to put it in its place in the multi-digit value
        ADD R3, R4, #0              ;R3 is gonna now hold the previous R4's Number
        ADD R3, R3, R3              
        ADD R3, R3, R3              
        ADD R3, R3, R3              
        ADD R3, R4, R3              
        ADD R3, R4, R3              
        
        ADD R4, R3, R0              ;Final Number :)
        
        ADD R1, R1, #1              ;move the array address to the next
        ADD R2, R2, #-1             
        BRp Num_Input_Loop
        
        QUIT

; remember to end with a newline!
    LD  R5, ASCII_NEG_SIGN
    NOT R5, R5
    ADD R5, R5, #1              ;R5 store "-45", use to check if the input is "-" or not

    LD R1, Arr_Pointer
    LDR R2, R1, #0
    ADD R2, R2, R5
    BRnp Insert_NewSpace                      ;if it's not negative number
        NOT R4, R4
        ADD R4, R4, #1

    Insert_NewSpace    
        LD R0, NewSpace
        OUT
BRnp ProgramHalt

ProgramHalt				
    HALT

;---------------	
; Program Data
;---------------

IntroPromptMessage  .FILL xB000
Error_Message .FILL xB200

ASCII_NEG_SIGN  .FILL    x2D    ;'-' in ascii code
NewSpace        .FILL    x0A    ;new line in ascii code
Hex_zero_val    .FILL    x30    ;'0' in ascii code
Hex_nine_val    .FILL    x39    ;'9' in ascii code

Arr_Pointer     .FILL    x4000    ;pointing to the array's address
Arr_size        .FILL    #6    ;Store maximum 6 symbols in the array

.END

;------------
; Remote data
;------------
.ORIG xB000	 ; intro prompt
.STRINGZ	 "Input a positive or negative decimal number (max 5 digits), followed by ENTER\n"

.END					
					
.ORIG xB200	 ; error message
.STRINGZ	 "ERROR: invalid input\n"

;---------------
; END of PROGRAM
;---------------
.END

;-------------------
; PURPOSE of PROGRAM
;-------------------
; Convert a sequence of up to 5 user-entered ascii numeric digits into a 16-bit two's complement binary representation of the number.
; if the input sequence is less than 5 digits, it will be user-terminated with a newline (ENTER).
; Otherwise, the program will emit its own newline after 5 input digits.
; The program must end with a *single* newline, entered either by the user (< 5 digits), or by the program (5 digits)
; Input validation is performed on the individual characters as they are input, but not on the magnitude of the number.
