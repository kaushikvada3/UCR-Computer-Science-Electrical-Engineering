;=================================================
; Name: 
; Email: 
; 
; Lab: lab 8, ex 1
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
; Subroutine: LOAD_FILL_VALUE_3200
; Parameter: // Fixme
; Postcondition: // Fixme
; Return Value: // Fixme
;=================================================

.orig x3200

; Backup registers

; Code

; Restore registers

RET

.end

;=================================================
; Subroutine: OUTPUT_AS_DECIMAL_3400
; Parameter: // Fixme
; Postcondition: // Fixme
; Return Value: // Fixme
;=================================================

.orig x3400

; Backup registers

; Code

; Restore registers

RET

.end