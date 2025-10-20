# Lab 3 Starter Code: Stack and Function Call Simulation
# ------------------------------------------------------
# Task: Complete the sum function using stack operations
# and demonstrate multiple function calls.

.data
    result:  .word 0             # First result
    result2: .word 0             # Second result

.text
main:
    li $a0, 5                    # First argument
    li $a1, 10                   # Second argument
    jal sum                      # Call sum function
    move $t0, $v0                # Store return value
    sw $v0, result               # Save to memory

    li $a0, 12                   # New arguments
    li $a1, 7
    jal sum                      # Call sum again
    move $t1, $v0
    sw $v0, result2              # Save second result

    li $v0, 10                   # Exit
    syscall

sum:
    addiu $sp, $sp, -4           # Allocate stack space
    sw $ra, 0($sp)               # Save return address

    addu $v0, $a0, $a1           # Compute sum

    lw $ra, 0($sp)               # Restore return address
    addiu $sp, $sp, 4            # Deallocate stack
    jr $ra                       # Return to caller