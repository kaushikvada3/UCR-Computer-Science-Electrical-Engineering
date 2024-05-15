;=========================================================================
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
;=========================================================================
; Busyness vector: xB600 

.ORIG x3000			; Program begins here
;-------------
;Instructions
;-------------
;-------------------------------
;INSERT CODE STARTING FROM HERE
;--------------------------------
                Main
                    LD R6, Subroutine_ptr       ;R6 hold the pointer to the subroutine block
                    
                    LDR R0, R6, #0
                    JSRR R0
                    
                    ;After the manu, R1 holds the users input(ascii code of the number!!)
                    LD R2, Hex_30_3000
                    NOT R2, R2
                    ADD R2, R2, #1              ; R2 = -48
                    
                    LD R3, Hex_37_3000
                    NOT R3, R3
                    ADD R3, R3, #1              ; R3 = -55
                    
                    ADD R3, R1, R3              ; R3 = R1 + R3
                    
                    ;====================================================================== 
                    ;OPTION 7, END THE PROGRAM
                    BRnp #1                     ; if r3 + r1 != 0 means the input is not equal to 7.
                        BR PROGRAM_END          ; end the program
                    ;======================================================================  
                    
                    ;check for all the rest of options
                    ADD R1, R1, R2              ;change r1 from ascii code to number
                    
                    ;CHECK which subroutine we should use
                    ADD r1, r1, #-1              ; r1 = r1 -1
                    BRp #10   
                    
                    ;====================================================================== 
                    ;OPTION 1, ALL_MACHINES_BUSY
                    LDR R0, R6, #1          ; r1 -1 = 0. means r1 = 1, go to the 1st sub.    
                    JSRR R0
                    AND R2, R2, R2          ;RETURN VALUE IS IN R2
                    BRz #3
                        LEA R0, allbusy
                        PUTS
                        BR Main
                    LEA R0, allnotbusy
                        PUTS
                    BR Main
                    ;====================================================================== 
                    
                    ADD r1, r1, #-1              ; r1 = r1 -1
                    BRp #10
                    ;====================================================================== 
                    ;OPTION 2, ALL_MACHINES_FREE
                    LDR R0, R6, #2          ; r1 -2 = 0. means r1 = 3, go to the 2nd sub. 
                    JSRR R0
                    AND R2, R2, R2          ;RETURN VALUE IS IN R2
                    BRz #3
                        LEA R0, allnotfree
                        PUTS
                        BR Main
                    LEA R0, allfree
                        PUTS
                    BR Main
                    ;====================================================================== 
                    
                    ADD r1, r1, #-1              ; r1 = r1 -1
                    BRp #9
                    ;====================================================================== 
                    ;OPTION 3, NUM_BUSY_MACHINES
                    LDR R0, R6, #3          ; r1 -3 = 0. means r1 = 3, go to the 3rd sub. 
                    JSRR R0                 ;now the value is stored in r1
                    
                    LEA R0, busymachine1
                    PUTS
                    LDR R0, R6, #8          ;call the sub. to print the number
                    JSRR R0
                    LEA R0, busymachine2
                    PUTS
                    BR Main
                    ;====================================================================== 
                    
                    ADD r1, r1, #-1              ; r1 = r1 -1
                    BRp #15
                    ;====================================================================== 
                    ;OPTION 4, NUM_FREE_MACHINES
                    LDR R0, R6, #3          ;here, the offset should be 4, but I use 16 - #of busy machine to get the answer.
                    JSRR R0                 ;now the value is stored in r1
                    
                    AND R0, R0, #0;
                    ADD R0, R0, #8;
                    ADD R0, R0, #8;
                    NOT R1, R1
                    ADD R1, R1, #1;        ;now r1 = -r1
                    ADD R1, R0, R1          ;get the number of free machine! 
                    
                    LEA R0, freemachine1
                    PUTS
                    LDR R0, R6, #8          ;call the sub. to print the number
                    JSRR R0
                    LEA R0, freemachine2
                    PUTS
                    BR Main
                    ;====================================================================== 
                    
                    ADD r1, r1, #-1              ; r1 = r1 -1
                    BRp #14
                    ;====================================================================== 
                    ;OPTION 5, MACHINE_STATUS
                    LDR R0, R6, #5          ;r1 -5 = 0. means r1 = 5, go to the 5TH sub. 
                    JSRR R0                 ;now the # of machine stored in r1, the status stored in r2 {(R2): 0 if machine (R1) is busy, 1 if it is free}
                    
                    LEA R0, status1
                    PUTS
                    
                    LDR R0, R6, #8          ;call the sub. to print the number
                    JSRR R0
                    
                    AND R2, R2, R2
                    BRz #3                  ;0 means machine is busy
                        LEA R0, status3
                        PUTS 
                        BR Main
                        
                    LEA R0, status2
                    PUTS 
                    BR Main
                    ;====================================================================== 
                    
                    BR Main

                PROGRAM_END 
                    LD R0, newline
                    OUT
                    LEA R0, goodbye
                    PUTS
                    
                    ;end with newline
                    LD R0, newline
                    OUT
HALT
;---------------	
;Data
;---------------
;Subroutine pointers
Subroutine_ptr  .Fill x4900     ;x4900 mem address hold all the ptr to the subroutine, because it's remote data, so I can use offset

;Other data 
newline 		     .Fill x0A
Hex_30_3000          .Fill x30   ;0's ascii code
Hex_37_3000          .FILL x37   ;7's ascii code

; Strings for reports from menu subroutines:
goodbye         .stringz "Goodbye!"
allbusy         .stringz "\nAll machines are busy\n"
allnotbusy      .stringz "\nNot all machines are busy\n"
allfree         .stringz "\nAll machines are free\n"
allnotfree		.stringz "\nNot all machines are free\n"
busymachine1    .stringz "\nThere are "
busymachine2    .stringz " busy machines\n"
freemachine1    .stringz "\nThere are "
freemachine2    .stringz " free machines\n"
status1         .stringz "\nMachine "
status2		    .stringz " is busy\n"
status3		    .stringz " is free\n"
firstfree1      .stringz "The first available machine is number "
firstfree2      .stringz "No machines are free\n"
.END



;-----------------------------------------------------------------------------------------------------------------
; Subroutine: MENU
; Inputs: None
; Postcondition: The subroutine has printed out a menu with numerical options, invited the
;                user to select an option, and returned the selected option.
; Return Value (R1): The option selected:  #1, #2, #3, #4, #5, #6 or #7 (as a number, not a character)
;                    no other return value is possible
;-----------------------------------------------------------------------------------------------------------------
.ORIG x5400
;-------------------------------
;INSERT CODE For Subroutine MENU
;--------------------------------
;HINT back up 
                    ST R2, backup_r2_5400     
                    ST R3, backup_r3_5400      
                    ST R4, backup_r4_5400     
                    ST R5, backup_r5_5400      
                    ST R6, backup_r6_5400      
                    ST R7, backup_r7_5400     
;---------------------------------------

                    LOOP_MENU
                        LD R0, Menu_string_addr
                        PUTS
                        
                        GETC                    ;GET USER'S INPUT
                        OUT
                        
                        ;CHECK IF THE INPUT IS VALID
                        ADD R2, R0, #0          ;copy the input into r2
                        
                        LD R3, Hex_37           ;let the r3 hold the -55
                        NOT R3, R3
                        ADD R3, R3, #1
                        
                        ADD R2, R3, R2          ;add the input with -55
                        BRnz #3                 ;if r2 + r3 <= 0 keep check the input, otherwise pop error
                            LEA R0, Error_msg_1
                            PUTS
                            BR LOOP_MENU        ;show the manu, let the user re-enter
                            
                        ADD R2, R2, #7          ;if r2 + r3 + 7 >=0, input is valid. 
                        BRp #3
                            LEA R0, Error_msg_1
                            PUTS
                            BR LOOP_MENU        ;show the manu, let the user re-enter
                        
                        ADD R1, R0, #0          ;Store the input to r1

;HINT Restore
                    LD R2, backup_r2_5400     
                    LD R3, backup_r3_5400      
                    LD R4, backup_r4_5400     
                    LD R5, backup_r5_5400      
                    LD R6, backup_r6_5400      
                    LD R7, backup_r7_5400  
                    
                    RET
;--------------------------------
;Data for subroutine MENU
;--------------------------------
backup_r2_5400      .blkw #1
backup_r3_5400      .blkw #1
backup_r4_5400      .blkw #1
backup_r5_5400      .blkw #1
backup_r6_5400      .blkw #1
backup_r7_5400      .blkw #1
Hex_37              .FILL x37   ;the ascii code for 7

Error_msg_1	      .STRINGZ "\nINVALID INPUT"
Menu_string_addr  .FILL x5000
.END



.ORIG x5600
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: ALL_MACHINES_BUSY (#1)
; Inputs: None
; Postcondition: The subroutine has returned a value indicating whether all machines are busy
; Return value (R2): 1 if all machines are busy, 0 otherwise
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine ALL_MACHINES_BUSY
;--------------------------------
;HINT back up 
                    ST R1, backup_r1_5600    
                    ST R3, backup_r3_5600      
                    ST R4, backup_r4_5600     
                    ST R5, backup_r5_5600      
                    ST R6, backup_r6_5600      
                    ST R7, backup_r7_5600    
;---------------------------------------

                    LD R2, BUSYNESS_ADDR_ALL_MACHINES_BUSY
                    LDR R0, R2, #0
                    BRz #2                  ;if r0 =0 ==> all machine are busy 
                        AND R2, R2, #0      ;not all machines are busy, set r1 = 0
                        Br FINISHED_5600
                    
                    AND R2, R2, #0             
                    ADD R2, R2, #1          ;set r1 =1, means all machine are busy.
                    
                    
;HINT Restore       
                    FINISHED_5600
                    LD R1, backup_r1_5600     
                    LD R3, backup_r3_5600      
                    LD R4, backup_r4_5600     
                    LD R5, backup_r5_5600      
                    LD R6, backup_r6_5600      
                    LD R7, backup_r7_5600  
;---------------------------------------

                    RET
;--------------------------------
;Data for subroutine ALL_MACHINES_BUSY
;--------------------------------
backup_r1_5600      .blkw #1
backup_r3_5600      .blkw #1
backup_r4_5600      .blkw #1
backup_r5_5600      .blkw #1
backup_r6_5600      .blkw #1
backup_r7_5600      .blkw #1  
BUSYNESS_ADDR_ALL_MACHINES_BUSY .Fill xB600
.END



.ORIG x5800
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: ALL_MACHINES_FREE (#2)
; Inputs: None
; Postcondition: The subroutine has returned a value indicating whether all machines are free
; Return value (R2): 1 if all machines are free, 0 otherwise
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine ALL_MACHINES_FREE
;--------------------------------
;HINT back up       
                    ST R1, backup_r1_5800    
                    ST R3, backup_r3_5800      
                    ST R4, backup_r4_5800     
                    ST R5, backup_r5_5800      
                    ST R6, backup_r6_5800      
                    ST R7, backup_r7_5800    
;---------------------------------------
                    ;machine state: 1 is free, 0 is 
                    
                    LD R2, BUSYNESS_ADDR_ALL_MACHINES_FREE
                    LDR R0, R2, #0
                    ADD R0, R0, #1          ;if all machine are free, r0=0, r0+1 = 0
                    BRz #3
                        AND R2, R2, #0
                        ADD R2, R2, #1      ;all machine are free, return r2 = 1 
                        BR FINISHED_5800
                    AND R2, R2, #0          ;not all machine are free, return r2 = 0

;HINT Restore
                    FINISHED_5800
                    LD R1, backup_r1_5800     
                    LD R3, backup_r3_5800      
                    LD R4, backup_r4_5800     
                    LD R5, backup_r5_5800      
                    LD R6, backup_r6_5800      
                    LD R7, backup_r7_5800  
;---------------------------------------

                    RET
;--------------------------------
;Data for subroutine ALL_MACHINES_FREE
;--------------------------------
backup_r1_5800      .blkw #1
backup_r3_5800      .blkw #1
backup_r4_5800      .blkw #1
backup_r5_5800      .blkw #1
backup_r6_5800      .blkw #1
backup_r7_5800      .blkw #1  
BUSYNESS_ADDR_ALL_MACHINES_FREE .Fill xB600
.END




.ORIG x6000
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: NUM_BUSY_MACHINES (#3)
; Inputs: None
; Postcondition: The subroutine has returned the number of busy machines.
; Return Value (R1): The number of machines that are busy (0)
;-----------------------------------------------------------------------------------------------------------------

;-------------------------------
;INSERT CODE For Subroutine NUM_BUSY_MACHINES
;--------------------------------
;HINT back up                     
                    ST R2, backup_r2_6000    
                    ST R3, backup_r3_6000      
                    ST R4, backup_r4_6000     
                    ST R5, backup_r5_6000      
                    ST R6, backup_r6_6000      
                    ST R7, backup_r7_6000    
;---------------------------------------
                    ;machine state: 1 is free, 0 is 
                    
                    LD R2, BUSYNESS_ADDR_NUM_BUSY_MACHINES
                    LDR R0, R2, #0          ;R0 HOLD THE STATES OF ALL MACHINES
                    
                    AND R1, R1, #0          ;R1 used to store the number of machine that are busy
                    
                    AND R3, R3, #0          ;R3 used for loop countdown
                    ADD R3, R3, #8
                    ADD R3, R3, #8          ;now r3 is 16. (do it twice is because lc3 doesn't allowe to add 16 )
                    
                    COUNT_LOOP
                        ADD R3, R3, #-1
                        BRn FINISHED_6000  ; out the loop when count down is <0
                        
                        AND R0, R0, R0
                        BRn #1              ;If the first digit of r0 is 1, means the machine is free, do nothing
                            ADD R1, R1, #1  ;ADD 1 to r1 if the first dight of r0 is 0
                        ADD R0, R0, R0      ;LEFT SHIFT
                        BR COUNT_LOOP
                        
;HINT Restore
                    FINISHED_6000
                    LD R2, backup_r2_6000     
                    LD R3, backup_r3_6000      
                    LD R4, backup_r4_6000     
                    LD R5, backup_r5_6000      
                    LD R6, backup_r6_6000      
                    LD R7, backup_r7_6000 
;---------------------------------------

                    RET
;--------------------------------
;Data for subroutine NUM_BUSY_MACHINES
;--------------------------------
backup_r2_6000      .blkw #1
backup_r3_6000      .blkw #1
backup_r4_6000      .blkw #1
backup_r5_6000      .blkw #1
backup_r6_6000      .blkw #1
backup_r7_6000      .blkw #1  
BUSYNESS_ADDR_NUM_BUSY_MACHINES .Fill xB600
.END





;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;I havn't code the num_free_machines because 16 - #of busy machine is the answer ....

;-----------------------------------------------------------------------------------------------------------------
; Subroutine: NUM_FREE_MACHINES (#4)
; Inputs: None
; Postcondition: The subroutine has returned the number of free machines
; Return Value (R1): The number of machines that are free (1)
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine NUM_FREE_MACHINES
;--------------------------------
;HINT back up 

;HINT Restore

;--------------------------------
;Data for subroutine NUM_FREE_MACHINES 
;--------------------------------
;BUSYNESS_ADDR_NUM_FREE_MACHINES     .Fill xB600






.ORIG x6400
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: MACHINE_STATUS (#5)
; Input (R1): Which machine to check, guaranteed in range {0,15}
; Postcondition: The subroutine has returned a value indicating whether
;                the selected machine (R1) is busy or not.
; Return Value (R2): 0 if machine (R1) is busy, 1 if it is free
;              (R1) unchanged
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine MACHINE_STATUS
;--------------------------------
;HINT back up                     
                    ST R3, backup_r3_6400      
                    ST R4, backup_r4_6400     
                    ST R5, backup_r5_6400      
                    ST R6, backup_r6_6400      
                    ST R7, backup_r7_6400    
;---------------------------------------
                    LD R0, GET_MACHINE_NUM_PTR
                    JSRR R0                     ;now r1 hold the # of machine
                    
                    AND R3, R3, #0              ;clear r3
                    ADD R3, R3, #15             ;r3 = 15
                    
                    NOT R0, R0
                    ADD R0, R0, #1              ;R0 = -R0
                    
                    ADD R3, R0, R3              ;R3 = R3 + R0 = R3 - R1 ==> after we left shilt r3 time, if the machine status is nagative means free, positive means busy
                    
                    LD R4, BUSYNESS_ADDR_MACHINE_STATUS
                    LDR R0, R4, #0              ;R0 store the machine status
                    
                    Shift_loop
                        AND R3, R3, R3
                        BRz End_Loop
                        ADD R0, R0, R0          ;left shift
                        ADD R3, R3, #-1
                        BR Shift_loop
                        
                    End_Loop
                    ;now check the status of machine
                    AND R0, R0, R0
                    BRn #2             ;if it's negative ==> machine free
                        AND R2, R2, #0  ;machine is busy
                        BR FINISH_6400
                    AND R2, R2, #0
                    ADD R2, R2, #1      ;machine is free
                    
;HINT Restore       
                    FINISH_6400
                    LD R3, backup_r3_6400      
                    LD R4, backup_r4_6400     
                    LD R5, backup_r5_6400      
                    LD R6, backup_r6_6400      
                    LD R7, backup_r7_6400 
;---------------------------------------

                    RET
;--------------------------------
;Data for subroutine MACHINE_STATUS
;--------------------------------
backup_r3_6400      .blkw #1
backup_r4_6400      .blkw #1
backup_r5_6400      .blkw #1
backup_r6_6400      .blkw #1
backup_r7_6400      .blkw #1  
BUSYNESS_ADDR_MACHINE_STATUS        .Fill xB600

GET_MACHINE_NUM_PTR         .Fill x6800;

.END





;-----------------------------------------------------------------------------------------------------------------
; Subroutine: FIRST_FREE (#6)
; Inputs: None
; Postcondition: The subroutine has returned a value indicating the lowest numbered free machine
; Return Value (R1): the number of the free machine
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine FIRST_FREE
;--------------------------------
;HINT back up 

;HINT Restore

;--------------------------------
;Data for subroutine FIRST_FREE
;--------------------------------
;BUSYNESS_ADDR_FIRST_FREE .Fill xB600




.ORIG x6800
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: GET_MACHINE_NUM
; Inputs: None
; Postcondition: The number entered by the user at the keyboard has been converted into binary,
;                and stored in R1. The number has been validated to be in the range {0,15}
; Return Value (R1): The binary equivalent of the numeric keyboard entry
; NOTE: You can use your code from assignment 4 for this subroutine, changing the prompt, 
;       and with the addition of validation to restrict acceptable values to the range {0,15}
;-----------------------------------------------------------------------------------------------------------------
;-------------------------------
;INSERT CODE For Subroutine 
;--------------------------------
;HINT back up       
                    ST R2, backup_r2_6800      
                    ST R3, backup_r3_6800      
                    ST R4, backup_r4_6800     
                    ST R5, backup_r5_6800      
                    ST R6, backup_r6_6800      
                    ST R7, backup_r7_6800    
;---------------------------------------
                                        
                    LD  R5, Hex_30
                    NOT R5, R5
                    ADD R5, R5, #1              ;R5 store "-48", use to check if the input is "0" or not

                    LD  R6, Hex_0A
                    NOT R6, R6
                    ADD R6, R6, #1              ;R6 store "-12", use to check if the input is newline or not
                    
; output intro prompt
                    Loop_Initial
                        LEA R0, prompt
                        PUTS  
                    
;---------------------------------------
;get the first number, the first imput cannot be newline   
                        GETC                        ;get the number
                        OUT
                        
                        ADD R3, R0, #0
                        ADD R3, R3, R5              ;R3 = R3 - 48
                        BRzp #3                     ;if r3-48 < 0 ==> error
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        ADD R3, R3, #-9             ;if r3 - 48 -9 >0 ==> error
                        BRnz #3
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        
                        ADD R0, R0, R5              ;now the r0 is valid "number", r0 = r0 -48
                        ST R0, First_num            ;STORE r0 into the first number

;get the second number, the first imput can be newline, and cannot be larger than 6
                        
                        GETC 
                        OUT
                        
                        ADD R3, R0, #0
                        ADD R3, R3, R6              ;R3 = R3 - 12
                        BRzp #3                     ;if r3 - 12 < 0 ==> error
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        
                        AND R3, R3, R3
                        BRp #1                      ;if the second imput is newline, end input
                            BR END_INPUT
                        
                        ;if the first number is already larger than 1, error!
                        LD R6, First_num
                        ADD R6, R6, #-1             ;if first# -1 > 0, so it's larger than 1,then no matter what the second number is, error
                        BRnz #3
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        
                        ADD R3, R0, #0              ;RESET R3
                        ADD R3, R3, R5              ;R3 = R3 - 48
                        BRzp #3                     ;if r3-48 < 0 ==> error
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        ADD R3, R3, #-5             ;if r3 - 48 -5 >0 ==> error (-5 is because the second input cannot be larger than 6)
                        BRnz #3
                            LEA R0, Error_msg_2
                            PUTS
                            BR Loop_Initial
                        
                        ADD R0, R0, R5              ;now the r0 is valid "number", r0 = r0 -48
                        ST R0, Second_num           ;STORE r0 into the second number
                    
;get the input number(2 valid number)
                    LD R0, First_num
                    ADD R1, R0, #0                  ;both r1 and r0 hold the first input number
                    ADD R0, R0, R0                  ;X2
                    ADD R0, R0, R0                  ;X4
                    ADD R0, R0, R0                  ;X8
                    ADD R0, R0, R1                  ;X9
                    ADD R0, R0, R1                  ;X10
                    
                    LD R1, Second_num
                    ADD R1, R0, R1                  ;R1 hold the input number
                    BR FINISH_6800
                    
;get the input number(1 valid number)  
                    END_INPUT
                    LD R1, First_num                ;R1 hold the input number
;HINT Restore       
                    FINISH_6800
                    LD R2, backup_r2_6800      
                    LD R3, backup_r3_6800      
                    LD R4, backup_r4_6800     
                    LD R5, backup_r5_6800      
                    LD R6, backup_r6_6800      
                    LD R7, backup_r7_6800 
;---------------------------------------

                    RET
;--------------------------------
;Data for subroutine Get input
;--------------------------------
backup_r2_6800      .blkw #1
backup_r3_6800      .blkw #1
backup_r4_6800      .blkw #1
backup_r5_6800      .blkw #1
backup_r6_6800      .blkw #1
backup_r7_6800      .blkw #1  
prompt .STRINGZ "\nEnter which machine you want the status of (0 - 15), followed by ENTER: "
Error_msg_2 .STRINGZ "\nERROR INVALID INPUT\n"

Hex_0A          .FILL x0A       ;new line in ascii code
Hex_30          .FILL x30       ;'0' in ascii code
Hex_35          .FILL x35       ;'5' in ascii code
Hex_39          .FILL x39       ;'9' in ascii code

First_num       .blkw #1
Second_num      .blkw #2
.END


.ORIG x8000	
;-----------------------------------------------------------------------------------------------------------------
; Subroutine: PRINT_NUM
; Inputs: R1, which is guaranteed to be in range {0,16}
; Postcondition: The subroutine has output the number in R1 as a decimal ascii string, 
;                WITHOUT leading 0's, a leading sign, or a trailing newline.
; Return Value: None; the value in R1 is unchanged
;-----------------------------------------------------------------------------------------------------------------

;-------------------------------
;INSERT CODE For Subroutine 
;--------------------------------
;HINT back up
                    ST R1, backup_r1_8000   
                    ST R2, backup_r2_8000     
                    ST R3, backup_r3_8000      
                    ST R4, backup_r4_8000     
                    ST R5, backup_r5_8000      
                    ST R6, backup_r6_8000      
                    ST R7, backup_r7_8000  
;---------------------------------------
                    ;step 1 check if the r1 is > 10
                    AND R2, R2, #0
                    
                    LD R3, Hex_30_8000
                    
                    ADD R2, R1, #-10
                    BRzp #4               ;if r1 = 10 < 0, means it's single digit
                        ADD R0, R1, #0
                        ADD R0, R0, R3
                        OUT
                        BR FINISHED_8000
                    
                    AND R2, R2, R2
                    Brp #7               ;if r2 was r1-10. if it's 0. means r1 = 10
                        AND R0, R0, #0
                        ADD R0, R0, #1
                        ADD R0, R0, R3
                        OUT
                        ADD R0, R0, #-1
                        OUT
                        BR FINISHED_8000
                        
                    AND R0, R0, #0
                    ADD R0, R0, #1
                    ADD R0, R0, R3
                    OUT
                    ADD R0, R2, #0      ;R2 hold the last dight of r1. r0 = r2
                    ADD R0, R0, R3
                    OUT
 
;HINT Restore       
                    FINISHED_8000
                    LD R1, backup_r1_8000   
                    LD R2, backup_r2_8000     
                    LD R3, backup_r3_8000      
                    LD R4, backup_r4_8000     
                    LD R5, backup_r5_8000      
                    LD R6, backup_r6_8000      
                    LD R7, backup_r7_8000  
;---------------------------------------
                    
                    RET
;--------------------------------
;Data for subroutine print number
;--------------------------------
backup_r1_8000      .blkw #1
backup_r2_8000      .blkw #1
backup_r3_8000      .blkw #1
backup_r4_8000      .blkw #1
backup_r5_8000      .blkw #1
backup_r6_8000      .blkw #1
backup_r7_8000      .blkw #1

Hex_30_8000         .Fill x30

.END


;remote data of point to subroutine, so I can use offset to go to the related subroutine.
.ORIG x4900     
MENU_PTR                .Fill x5400 ;offset 0
ALL_MACHINES_BUSY_PTR   .Fill x5600 ;offset 1 
ALL_MACHINES_FREE       .Fill x5800 ;offset 2
NUM_BUSY_MACHINES       .Fill x6000 ;offset 3
NUM_FREE_MACHINES       .Fill x6200 ;offset 4
MACHINE_STATUS          .Fill x6400 ;offset 5
FIRST_FREE              .Fill x6600 ;offset 6
GET_MACHINE_NUM         .Fill x6800 ;offset 7
PRINT_NUM               .Fill x8000 ;offset 8
.END 

.ORIG x5000
MENUSTRING      .STRINGZ "**********************\n* The Busyness Server *\n**********************\n1. Check to see whether all machines are busy\n2. Check to see whether all machines are free\n3. Report the number of busy machines\n4. Report the number of free machines\n5. Report the status of machine n\n6. Report the number of the first available machine\n7. Quit\n"
.END


.ORIG xB600			        ; Remote data
BUSYNESS .FILL x0000	; <----!!!BUSYNESS VECTOR!!! Change this value to test your program.
.END

;---------------	
;END of PROGRAM
;---------------	

