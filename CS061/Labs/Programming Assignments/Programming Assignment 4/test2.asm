;=========================================================================
; Name & Email must be EXACTLY as in Gradescope roster!
; Name: Yu Guo  
; Email: yguo139@ucr.edu
; 
; Assignment name: Assignment 4
; Lab section: 
; TA: Samuel Tapia
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
Loop_Initial
    LD R0, introPromptPtr
    PUTS 

; Set up flags, counters, accumulators as needed
    LD R1, ARRAY_PTR
    LD R2, ARRAY_SIZE

    AND R4, R4, #0              ;clear r4, use it to store the final result
    
    AND R5, R5, #0
    AND R6, R6, #0
    
    LD  R5, Hex_2D
    NOT R5, R5
    ADD R5, R5, #1              ;R5 store "-45", use to check if the input is "-" or not

    LD  R6, Hex_0A
    NOT R6, R6
    ADD R6, R6, #1              ;R6 store "-10", use to check if the input is "\n" or not
    
    AND R3, R3, #0              ;set the r3 to 0. Use r3 to check the input

; Get first character, test for '\n', '+', '-', digit/non-digit
    GETC
    OUT
    
    ADD R3, R0, R5
    BRz HandleNegativeSign      ; If R3 = 0, which means the input is '-', store the negative sign into the first array address
    ADD R3, R3, #2          
    BRz HandlePositiveSign      ; If R3 = 0 now, the input is '+'

; is very first character = '\n'? if so, just quit (no message)!
    AND R3, R3, #0
    ADD R3, R6, R0
    BRz EndProgram              ; If the input symbol's ascii code is 10(newline) terminate the program

    BRnp CheckDigit             ; if it's != '\n' & '-' & '+' go check digit    

HandleNegativeSign
    STR R0, R1, #0              ; Store the negative sign
    ADD R1, R1, #1              ; move the array address to the next
    ADD R2, R2, #-1             ; left 5 space to enter the number
    BR ContinueProcessing       ; skip the next few steps(recognize the input is + and check the digits)

HandlePositiveSign
    ADD R2, R2, #-1             ; left 5 space to enter the number
    BR ContinueProcessing       ; skip the next few steps(check the digits)

CheckDigit
; is it < '0'? if so, it is not a digit	- o/p error message, start over
    LD  R5, Hex_30
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-48", use to check if the input is "0" or not
    
    ADD R3, R0, R5
    BRzp IsGreaterThan9         ; if >'0', go check if it's >9
    LD R0, errorMessagePtr
    PUTS 
    BRnzp Loop_Initial

IsGreaterThan9
; is it > '9'? if so, it is not a digit	- o/p error message, start over
    LD  R5, Hex_39
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-57", use to check if the input is "9" or not
    
    ADD R3, R0, R5
    BRnz StoreValue             ; if it's <'9' store the value
    LD R0, errorMessagePtr
    PUTS 
    BRnzp Loop_Initial

StoreValue
    STR R0, R1, #0
    ADD R4, R4, R0              ; store the input value into R4
    ADD R1, R1, #1              ; move the array address to the next
    ADD R2, R2, #-1             ; left 4 space to enter the number

; if none of the above, first character is first numeric digit - convert it to number & store in target register!
    LD  R5, Hex_30
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-48", use to check if the input is "0" or not
    
    ADD R4, R4, R5              ; NOW R4 hold the real input number

ContinueProcessing
    AND R4, R4, R4              ; after check + and - go to this step

; Now get remaining digits from user in a loop (max 5), testing each to see if it is a digit, and build up number in accumulator
Loop_input
    GETC
    OUT
    
    AND R3, R3, #0
    ADD R3, R6, R0
    BRz EndInputLoop            ; If the input symbol's ascii code is 10(newline) end the input loop
    
    LD  R5, Hex_30
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-48", use to check if the input is "0" or not

    ADD R3, R0, R5
    BRzp IsDigitGreaterThan9    ; if >'0', go check if it's >9
    LD R0, errorMessagePtr
    PUTS 
    BRnzp Loop_Initial

IsDigitGreaterThan9
    LD  R5, Hex_39
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-57", use to check if the input is "9" or not

    ADD R3, R0, R5
    BRnz StoreDigit             ; if it's <'9' store the value
    LD R0, errorMessagePtr
    PUTS 
    BRnzp Loop_Initial

StoreDigit
    STR R0, R1, #0              ; Store the input into the array
    
    LD  R5, Hex_30
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-48"
    ADD R0, R5, R0              ; NOW R0 hold the real input number
    
    AND R3, R3, #0
    
    ADD R3, R4, #0              ; Now r3 hold the previous R4's number
    ADD R3, R3, R3              ; x2
    ADD R3, R3, R3              ; x4
    ADD R3, R3, R3              ; x8
    ADD R3, R4, R3              ; x9
    ADD R3, R4, R3              ; x10
    
    ADD R4, R3, R0              ; NEWEST NUMBER
    
    ADD R1, R1, #1              ; move the array address to the next
    ADD R2, R2, #-1             ; left 4 space to enter the number
    BRp Loop_input

EndInputLoop
; remember to end with a newline!
    LD  R5, Hex_2D
    NOT R5, R5
    ADD R5, R5, #1              ; R5 store "-45", use to check if the input is "-" or not
    
    LD R1, ARRAY_PTR
    LDR R2, R1, #0
    ADD R2, R2, R5
    BRnp NotNegative            ; if it's not a negative number
    NOT R4, R4
    ADD R4, R4, #1

NotNegative
    LD R0, Hex_0A
    OUT
HALT

EndProgram
    HALT

;---------------	
; Program Data
;---------------
introPromptPtr  .FILL xB000
errorMessagePtr .FILL xB200

Hex_2D          .FILL x2D       ; '-' in ASCII code
Hex_0A          .FILL x0A       ; newline in ASCII code
Hex_30          .FILL x30       ; '0' in ASCII code
Hex_39          .FILL x39       ; '9' in ASCII code

ARRAY_PTR       .FILL x4000     ; pointing to the array's address
ARRAY_SIZE      .FILL #6        ; Store maximum 6 symbols in the array
.END

;------------
; Remote data
;------------
.ORIG xB000	 ; intro prompt
.STRINGZ	 "\nInput a positive or negative decimal number (max 5 digits), followed by ENTER\n"
.END					
					
.ORIG xB200	 ; error message
.STRINGZ	 "\nERROR: invalid input\n"
.END

.ORIG x4000
ARRAY       .BLKW #6
;---------------
; END of PROGRAM
;---------------
.END

;-------------------
; PURPOSE of PROGRAM
;-------------------
; Convert a sequence of up to 5 user-entered ASCII numeric digits into a 16-bit two's complement binary representation of the number.
; if the input sequence is less than 5 digits, it will be user-terminated with a newline (ENTER).
; Otherwise, the program will emit its own newline after 5 input digits.
; The program must end with a *single* newline, entered either by the user (< 5 digits), or by the program (5 digits)
; Input validation is performed on the individual characters as they are input, but not on the magnitude of the number.
