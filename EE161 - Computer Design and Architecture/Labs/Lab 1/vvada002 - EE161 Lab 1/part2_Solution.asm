# Lab 2 Starter Code: Debugging a Loop
# -------------------------------------
# Task: Fix the invalid instruction in the loop so it correctly computes
# the sum of all array elements.

.data
    arr:     .word 1, 4, 7, 2, 5     # initial array
    n:       .word 5                 # length
    result:  .word 0                 # sum of ORIGINAL elements
    maxval:  .word 0                 # max among DOUBLED elements

.text
.globl main
main:
    la   $t0, arr                    # t0 = &arr[0]
    lw   $t1, n                      # t1 = count
    li   $t2, 0                      # t2 = sum (originals)
    li   $t5, 0x80000000             # t5 = current max (INT_MIN)

loop:
    beq  $t1, $zero, done
    lw   $t3, 0($t0)                 # t3 = A[i]

    # sum original
    addu $t2, $t2, $t3               # sum += A[i]

    # double in place
    sll  $t4, $t3, 1                 # t4 = A[i] * 2
    sw   $t4, 0($t0)                 # A[i] = doubled

    # max over DOUBLED values: if (t4 > t5) t5 = t4
    slt  $t6, $t5, $t4               # t6 = (t5 < t4)
    beq  $t6, $zero, skip_max
    move $t5, $t4
skip_max:
    addi $t0, $t0, 4                 # next element
    addi $t1, $t1, -1                # --count
    j    loop

done:
    sw   $t2, result                 # result = 1+4+7+2+5 = 19
    sw   $t5, maxval                 # maxval = max{2,8,14,4,10} = 14

    li   $v0, 10
    syscall