;=================================================
; Name: 
; Email: 
; 
; Lab: lab 8, ex 2
; Lab section: 
; TA: 
; 
;=================================================

.orig x3000

LD R6, top_stack_addr

; Test harness
;-------------------------------------------------

HALT

; Test harness local data
;-------------------------------------------------
top_stack_addr .fill xFE00

.end

;=================================================
; Subroutine: PARITY_CHECK_3600
; Parameter: // Fixme
; Postcondition: // Fixme
; Return Value (R3): // Fixme
;=================================================

.orig x3600

; Backup registers

; Code

; Restore registers

RET

.end