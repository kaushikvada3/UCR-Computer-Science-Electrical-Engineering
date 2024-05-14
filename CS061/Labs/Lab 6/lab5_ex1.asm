;=================================================
; Name: Kaushik Vada
; Email: vvada002@ucr.edu   
; 
; Lab: lab 5, ex 1
; Lab section: 024  
; TA: Karan and Cody
; 
;=================================================
.orig x3000
; Initialize the stack. Don't worry about what that means for now.
ld r6, top_stack_addr ; DO NOT MODIFY, AND DON'T USE R6, OTHER THAN FOR BACKUP/RESTORE

; your code goes here
;------------------------------------------------------------------------------------------
; Subroutine: SUB_STACK_PUSH
; Parameter (R1): The value to push onto the stack
; Parameter (R3): BASE: A pointer to the base (one less than the lowest available address) of the stack
; Parameter (R4): MAX: The "highest" available address in the stack
; Parameter (R5): TOS (Top of Stack): A pointer to the current top of the stack
; Postcondition: The subroutine has pushed (R1) onto the stack (i.e to address TOS+1). 
;		    If the stack was already full (TOS = MAX), the subroutine has printed an
;		    overflow error message and terminated.
; Return Value: R5 ← updated TOS
;------------------------------------------------------------------------------------------


halt

; your local data goes here

top_stack_addr .fill xFE00 ; DO NOT MODIFY THIS LINE OF CODE
.end

; your subroutines go below here










.orig x3000
; Initialize the stack. Don't worry about what that means for now.
ld r6, top_stack_addr ; DO NOT MODIFY, AND DON'T USE R6, OTHER THAN FOR BACKUP/RESTORE

; Subroutine to push a value onto the stack
; R1 = Value to push
; R3 = BASE pointer
; R4 = MAX pointer
; R5 = TOS (Top of Stack)

; Check if stack is full
add r2, r5, #1        ; Calculate next TOS
not r0, r4            ; Invert MAX
add r0, r0, #1        ; Add 1 to inverted MAX
add r0, r0, r5        ; Add TOS to (MAX + 1), Result should be 0 if TOS == MAX
brz STACK_FULL        ; Branch if result is zero (stack is full)

; Push the value onto the stack
str r1, r2, #0        ; Store R1 at address in R2 (TOS + 1)
add r5, r5, #1        ; Update TOS
brnzp RETURN          ; Go to RETURN label

; Stack full handler
STACK_FULL:
puts                   ; Assume a routine that prints an error
ldr r0, error_msg      ; Load error message
putsp                 ; Print the error message
halt                  ; Stop execution

RETURN:
st r5, TOS_updated    ; Store updated TOS
ld r5, TOS_updated    ; Load updated TOS back to R5

halt

; Local data and message definitions
top_stack_addr .fill xFE00 ; Stack top address, DO NOT MODIFY
TOS_updated .blkw 1       ; Space for updated TOS
error_msg .stringz "Stack overflow error\n"

; your subroutines go below here
.end
