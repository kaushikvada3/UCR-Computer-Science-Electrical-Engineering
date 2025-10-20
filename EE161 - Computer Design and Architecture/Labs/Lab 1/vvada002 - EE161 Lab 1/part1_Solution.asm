
# Lab 1: Registers and Arithmetic Operations
# -------------------------------------------

.data
    num1: .word 5          # Define num1 in memory with value 5
    num2: .word 10         # Define num2 in memory with value 10

.text
    lw $t0, num1           # Load value of num1 into register $t0
    lw $t1, num2           # Load value of num2 into register $t1

    # TODO: Swap the contents of $t0 and $t1 without using an extra register
    xor  $t0, $t0, $t1  # t0 = t0 ^ t1
    xor  $t1, $t0, $t1  # t1 = (t0 ^ t1) ^ t1 = t0(orig)
    xor  $t0, $t0, $t1  # t0 = (t0 ^ t1) ^ t0(orig) = t1(orig)
    
    # TODO: Add the swapped values ($t0 + $t1) and store the result in $t2
    addu $t2, $t0, $t1
    
    # TODO: Subtract $t1 from $t0 (i.e., $t0 - $t1) and store in $t3
    subu $t3, $t0, $t1
    
    # TODO: Multiply $t0 and $t1 and store the result in $t4
    mult $t0, $t1
    mflo $t4
    

    li $v0, 10              # Exit program (MIPS syscall code for exit)
    syscall

