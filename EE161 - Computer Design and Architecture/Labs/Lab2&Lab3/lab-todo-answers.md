# MIPS Datapath Lab — ToDo Answers

Notes for the Logisim implementation based on “MIPS Datapath Lab-CS161.pdf”. Use these to finish the Logisim circuits and the written report.

## ToDo #1 — Instruction Type Decode
- Opcodes to detect with equality comparators (6-bit):
  - R-format: `000000` (already hinted)
  - lw: `100011` (0x23)
  - sw: `101011` (0x2b)
  - beq: `000100` (0x04)
- Feed each comparator’s “=” output to the corresponding R-format, lw, sw, and beq outputs of the type-decode block.

## ToDo #2 — Control Decode Truth Table

| Output    | R-format | lw | sw | beq |
|-----------|----------|----|----|-----|
| RegDst    | 1        | 0  | X  | X   |
| ALUSrc    | 0        | 1  | 1  | 0   |
| MemtoReg  | 0        | 1  | X  | X   |
| RegWrite  | 1        | 1  | 0  | 0   |
| MemRead   | 0        | 1  | 0  | 0   |
| MemWrite  | 0        | 0  | 1  | 0   |
| Branch    | 0        | 0  | 0  | 1   |
| ALUOp1    | 1        | 0  | 0  | 0   |
| ALUOp0    | 0        | 0  | 0  | 1   |

## ToDo #3 — Control-Decode Logic
Boolean equations derived from the table (you can enter these via Combinational Analysis or wire gates directly):
- RegDst = R-format
- ALUSrc = lw + sw
- MemtoReg = lw
- RegWrite = R-format + lw
- MemRead = lw
- MemWrite = sw
- Branch = beq
- ALUOp1 = R-format
- ALUOp0 = beq

## ToDo #4 — Wire Control Logic
Connect the type-decode outputs to the control-decode inputs in order (R-format → first pin, lw → second, sw → third, beq → fourth). Then route control outputs to:
- RegDst → write-register MUX (select between `instr[20:16]` and `instr[15:11]`).
- ALUSrc → ALU B-input MUX (register vs. sign-extended immediate).
- MemtoReg → write-back MUX (ALU result vs. data memory output).
- RegWrite → register file `RegWrite`.
- MemRead/MemWrite → data memory control pins.
- Branch → branch/PC selection logic (with Zero).
- ALUOp[1:0] → ALU Control block.

## ToDo #5 — ALU Control Logic (priority-encoder inputs)
Conditions that assert each priority-encoder input (driving constants 0,1,2,6,7 into the MUX). `F3..F0` are the low 4 bits of the function field.
- ADD (0010, already started): `(~ALUOp1 & ~ALUOp0)  OR  (ALUOp1 & ~ALUOp0 & ~F3 & ~F2 & ~F1 & ~F0)`
- SUB (0110): `(~ALUOp1 & ALUOp0)  OR  (ALUOp1 & ~ALUOp0 & ~F3 & ~F2 & F1 & ~F0)`
- AND (0000): `(ALUOp1 & ~ALUOp0 & ~F3 & F2 & ~F1 & ~F0)`
- OR  (0001): `(ALUOp1 & ~ALUOp0 & ~F3 & F2 & ~F1 & F0)`
- SLT (0111): `(ALUOp1 & ~ALUOp0 & F3 & ~F2 & F1 & ~F0)`
Wire each condition to a separate priority-encoder input, matching the constant (0 → AND, 1 → OR, 2 → ADD, 6 → SUB, 7 → SLT).

## ToDo #6 — Datapath Wiring Checklist
- Instruction path: PC → Instruction Memory address. Split instruction: `op` → Control, `rs`→RegFile read1, `rt`→RegFile read2 & RegDst MUX input0, `rd`→RegDst MUX input1, `funct`→ALU Control, `imm[15:0]`→Sign Extend.
- Register file: Write register from RegDst MUX output; Write data from MemtoReg MUX output; RegWrite from control.
- ALU: Input A from RegFile read1; Input B from ALUSrc MUX (read2 vs. sign-extended imm); ALU control from ALU Control block; Zero output feeds branch logic; result feeds data memory address and MemtoReg MUX.
- Data memory: Address = ALU result; Write data = RegFile read2; MemRead/MemWrite from control.
- Branch/PC logic: Sign-extended imm → shift-left-2 → branch adder; branch select uses Branch control AND Zero to choose between PC+4 and branch target (top “Next PC” wiring supplied in the starter).
- Write-back: MemtoReg MUX selects between data memory output and ALU result into RegFile write data.

## ToDo #7 — Final Register Values after `test-code.mem`
Running the provided program with `1234.mem` (1,2,3,4 in memory) yields after completion:
- $t0 = 0
- $t1 = 1
- $t2 = 2
- $t3 = 3
- $t4 = 4
- $t5 = 5
- $t6 = 1
In the register-file view you should see these values once the branch falls through after the second loop iteration.
