;=================================================
; Name: Yu Guo
; Email: yguo139@ucr.edu 
; 
; Lab: lab 9, ex 1
; Lab section: 
; TA: Samuel Tapia 
; 
;=================================================

; test harness
.orig x3000
            LD R6, SUB_LOAD_VALUE_ptr
            JSRR R6                         ;return r1

			ADD R1, R1, #1
			LD R6, SUB_PRINT_DECIMAL_ptr
			JSRR R6
		    
halt
;-----------------------------------------------------------------------------------------------
; test harness local data:


;--------------------
;subroutines_address
;--------------------
SUB_LOAD_VALUE_ptr      .FILL x3200
SUB_PRINT_DECIMAL_ptr   .FILL x3400

;===============================================================================================
.end





; subroutines:

;------------------------------------------------------------------------------------------
; Subroutine: SUB_LOAD_VALUE (Be sure to fill details in below)
;
; Parameter (R):  NONE
; Return Value:   R1 (The hardcoded value)
;------------------------------------------------------------------------------------------
.orig x3200
;BACK UP:
            ST R2, backup_r2_3200    
            ST R3, backup_r3_3200     
            ST R4, backup_r4_3200
            ST R5, backup_r5_3200
            ST R6, backup_r6_3200  
            ST R7, backup_r7_3200  
				 
		    LD R1, Num
				 
;restore            
            LD R2, backup_r2_3200     
            LD R3, backup_r3_3200
            LD R4, backup_r4_3200 
            LD R5, backup_r5_3200
            LD R6, backup_r6_3200 
            LD R7, backup_r7_3200	 
ret
;-----------------------------------------------------------------------------------------------
; SUB_LOAD_VALUE local data
backup_r2_3200  .BLKW #1
backup_r3_3200  .BLKW #1
backup_r4_3200  .BLKW #1
backup_r5_3200  .BLKW #1
backup_r6_3200  .BLKW #1
backup_r7_3200  .BLKW #1

Num             .FILL #1245
.end






;===============================================================================================


;------------------------------------------------------------------------------------------
; Subroutine: SUB_PRINT_DECIMAL (Be sure to fill details in below)
;
; Parameter (R): R1 (BINARY NUMBER)
; Return Value: NONE
;------------------------------------------------------------------------------------------
.orig x3400
;BACK UP:   
            ST R1, backup_r1_3400
            ST R2, backup_r2_3400    
            ST R3, backup_r3_3400     
            ST R4, backup_r4_3400
            ST R5, backup_r5_3400
            ST R6, backup_r6_3400  
            ST R7, backup_r7_3400  
		    
		    AND R1, R1, R1
		    brzp #4                      ;if r1 < 0, then output -sign, and set r1 = -r1
		        ADD R1, R1, #-1
		        NOT R1, R1
		        LD R0, hex_2D
		        OUT
		    
		    
		    LD R2, num_10000s           ;r2 = -10000
		    LD R3, num_0                ;R3 = 48
		    AND R0, R0, #0              ;R0 is the counter
		    
		     
		    _10000s_place_loop           ;SIMULATE R1/10000
                ADD R1, R1, R2          ;R1-10000
                BRzp #9                 ;if r1-10000 >=0, counter +=1, 
                    ADD R2, R2, #-1     ;if r1-10000 <0, reset r1(r1+10000)
                    NOT R2, R2          
                    ADD R1, R1, R2
                    AND R0, R0, R0      ;get the counter
                    BRz #2              ;if the counter = 0(the r1 was smaller than 10000 at the beginning), no output number
                        ADD R0, R0, R3
                        OUT
                    ADD R0, R0, R3
                    BR _1000s_place_loop_pre
                ADD R0, R0, #1
                BR _10000s_place_loop
                    
                    
            ;-----------------------------------------------------------        
			_1000s_place_loop_pre
			LD R2, num_1000s            ;r2 = -1000
			
			NOT R3, R3
			ADD R3, R3, #1              ;R3 = -48
			
			ADD R0, R0, R3              ;R0 = R0 -48
			BRp #2                      
			    ADD R6, R0, #1          ;if r0 = 0, r6 = 1. if 1000s place is also 0, we have to check 10000s place to decide weather to output this 0
			    BR #2
			AND R6, R6, #0              ;reset r6 to 0
			AND R0, R0, #0              ;reset r0 to 0

			_1000s_place_loop            ;SIMULATE R1/1000
                ADD R1, R1, R2          ;R1-1000
                BRzp #17                 ;if r1-1000 >=0, counter +=1, 
                    ADD R2, R2, #-1     ;if r1-1000 <0, reset r1(r1+1000)
                    NOT R2, R2          
                    ADD R1, R1, R2
                    AND R0, R0, R0      ;get the counter
                    BRz #4              ;if the counter = 0(the r1 was smaller than 1000 at the beginning of 1000s_place_loop)
                        LD R3, num_0    ;reset r3 = 48   
                        ADD R0, R0, R3
                        OUT
                        BR _100s_place_loop_pre
                        
                    AND R6, R6, R6
                    Brp #3                
                        LD R3, num_0    ;if r6 = 0, then 10000s place is not = 0 output the number. else dont print the number
                        ADD R0, R0, R3
                        OUT
                    LD R3, num_0                ;R3 = 48
                    ADD R0, R0, R3
                    BR _100s_place_loop_pre
                    
                ADD R0, R0, #1
                BR _1000s_place_loop
		    
		    ;-----------------------------------------------------------        
			_100s_place_loop_pre
			LD R2, num_100s            ;r2 = -100
			
			NOT R3, R3
			ADD R3, R3, #1              ;R3 = -48
			
			ADD R0, R0, R3              ;R0 = R0 - 48
			BRp #2                      
			    ADD R6, R0, #1          ;if r0 = 0, r6 = 1. if 100s place is also 0, we have to check 1000s place to decide weather to output this 0
			    BR #2
			AND R0, R0, #0              ;reset r0 to 0
			AND R6, R6, #0              ;reset r6 to 0
			
			_100s_place_loop            ;SIMULATE R1/100
                ADD R1, R1, R2          ;R1-1000
                BRzp #17                 ;if r1-100 >=0, counter +=1, 
                    ADD R2, R2, #-1     ;if r1-100 <0, reset r1(r1+100)
                    NOT R2, R2          
                    ADD R1, R1, R2
                    AND R0, R0, R0      ;get the counter
                    BRz #4              ;if the counter = 0(the r1 was smaller than 1000 at the beginning of 1000s_place_loop)
                        LD R3, num_0    ;reset r3 = 48   
                        ADD R0, R0, R3
                        OUT
                        BR _10s_place_loop_pre
                        
                    AND R6, R6, R6
                    Brp #3                
                        LD R3, num_0    ;if r6 = 0, then 1000s place is not = 0 output the number. else dont print the number
                        ADD R0, R0, R3
                        OUT
                    LD R3, num_0                ;R3 = 48
                    ADD R0, R0, R3
                    BR _10s_place_loop_pre
                    
                ADD R0, R0, #1
                BR _100s_place_loop
                
		    ;-----------------------------------------------------------  
		    _10s_place_loop_pre
			LD R2, num_10s            ;r2 = -10
			
			LD R3, num_0    ;reset r3 = 48 
			NOT R3, R3
			ADD R3, R3, #1              ;R3 = -48
			
			ADD R0, R0, R3              ;R0 = R0 - 48
			BRp #2                      
			    ADD R6, R0, #1          ;if r0 = 0, r6 = 1. if 10s place is also 0, we have to check 100s place to decide weather to output this 0
			    BR #2
			AND R0, R0, #0              ;reset r0 to 0
			AND R6, R6, #0              ;reset r6 to 0
			
			_10s_place_loop              ;SIMULATE R1/10
                ADD R1, R1, R2          ; R1-10
                BRzp #17                ;if r1-10 >=0, counter +=1, 
                    ADD R2, R2, #-1     ;if r1-10 <0, reset r1(r1+10)
                    NOT R2, R2          
                    ADD R1, R1, R2
                    AND R0, R0, R0      ;get the counter
                    BRz #4              ;if the counter = 0(the r1 was smaller than 1000 at the beginning of 100s_place_loop)
                        LD R3, num_0    ;reset r3 = 48   
                        ADD R0, R0, R3
                        OUT
                        BR _1s_place_loop_pre
                        
                    AND R6, R6, R6
                    Brp #3                
                        LD R3, num_0    ;if r6 = 0, then 100s place is not = 0 output the number. else dont print the number
                        ADD R0, R0, R3
                        OUT
                    LD R3, num_0                ;R3 = 48
                    ADD R0, R0, R3
                    BR _1s_place_loop_pre
                    
                ADD R0, R0, #1
                BR _10s_place_loop
                
		    ;-------------------------------
		    _1s_place_loop_pre
		    ;now r1 is in range [0,9], just print it
		    
		    LD R3, num_0
		    ADD R0, R1, R3
		    OUT
		    
		    
;restore    
            LD R1, backup_r1_3400
            LD R2, backup_r2_3400     
            LD R3, backup_r3_3400
            LD R4, backup_r4_3400 
            LD R5, backup_r5_3400
            LD R6, backup_r6_3400 
            LD R7, backup_r7_3400
            
ret
;-----------------------------------------------------------------------------------------------
; SUB_PRINT_DECIMAL local data
backup_r1_3400  .BLKW #1
backup_r2_3400  .BLKW #1
backup_r3_3400  .BLKW #1
backup_r4_3400  .BLKW #1
backup_r5_3400  .BLKW #1
backup_r6_3400  .BLKW #1
backup_r7_3400  .BLKW #1

hex_2D          .FILL x2d   ;"-"
num_0           .FILL #48
num_10000s      .FILL #-10000
num_1000s       .FILL #-1000
num_100s        .FILL #-100
num_10s         .FILL #-10
num_1s          .FILL #-1
.end

