#!/usr/bin/env python3
"""
Rigorous test suite for the Tomasulo CPU simulator.
Tests functional correctness, timing (cycle counts), edge cases, and hazards.
"""

import subprocess
import sys
import tempfile
import os
import re
import math

SIMULATOR_PATH = os.path.join(os.path.dirname(__file__), 'simulation.py')

passed = []
failed = []


def run_simulator(program_text, extra_args=None):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.dat', delete=False) as f:
        f.write(program_text)
        fname = f.name
    try:
        cmd = [sys.executable, SIMULATOR_PATH, fname]
        if extra_args:
            cmd.extend(extra_args)
        result = subprocess.run(cmd, capture_output=True, text=True)
        return result.stdout, result.stderr, result.returncode
    finally:
        os.unlink(fname)


def parse_output(output):
    s = {}
    m = re.search(r'Total execution cycles: (\d+)', output)
    if m:
        s['cycles'] = int(m.group(1))
    m = re.search(r'Issue stall events.*?: (\d+)', output)
    if m:
        s['stalls'] = int(m.group(1))
    s['R'] = {}
    for m in re.finditer(r'\bR(\d+)=(-?\d+)', output):
        s['R'][int(m.group(1))] = int(m.group(2))
    s['F'] = {}
    for m in re.finditer(r'\bF(\d+)=(-?[\d\.]+)', output):
        s['F'][int(m.group(1))] = float(m.group(2))
    s['M'] = {}
    for m in re.finditer(r'\[(\d+)\] = (-?[\d\.]+)', output):
        s['M'][int(m.group(1))] = float(m.group(2))
    return s


def check_test(name, program, expected_R=None, expected_F=None,
               expected_M=None, expected_cycles=None,
               expected_stalls=None, extra_args=None):
    stdout, stderr, rc = run_simulator(program, extra_args)
    if rc != 0:
        print(f"FAIL [{name}]: crashed — {stderr.strip()[:120]}")
        failed.append(name)
        return
    s = parse_output(stdout)
    errs = []
    if expected_R:
        for reg, val in expected_R.items():
            got = s['R'].get(reg)
            if got != val:
                errs.append(f"R{reg}: want {val}, got {got}")
    if expected_F:
        for reg, val in expected_F.items():
            got = s['F'].get(reg, 0.0)
            if not math.isclose(got, val, rel_tol=1e-6, abs_tol=1e-9):
                errs.append(f"F{reg}: want {val}, got {got}")
    if expected_M:
        for addr, val in expected_M.items():
            got = s['M'].get(addr, 0.0)
            if not math.isclose(got, val, rel_tol=1e-6, abs_tol=1e-9):
                errs.append(f"mem[{addr}]: want {val}, got {got}")
    if expected_cycles is not None:
        got = s.get('cycles')
        if got != expected_cycles:
            errs.append(f"cycles: want {expected_cycles}, got {got}")
    if expected_stalls is not None:
        got = s.get('stalls')
        if got != expected_stalls:
            errs.append(f"stalls: want {expected_stalls}, got {got}")
    if errs:
        print(f"FAIL [{name}]")
        for e in errs:
            print(f"    {e}")
        failed.append(name)
    else:
        print(f"PASS [{name}]")
        passed.append(name)


# ─────────────────────────────────────────────────────────────────
# SECTION 1: TIMING — hand-traced cycle counts
# ─────────────────────────────────────────────────────────────────
# Single addi: fetch(1) → issue(2) → start+finish(3) → WB(4) = 4 cycles
check_test("timing_single_addi",
    "addi R1, R0, 5",
    expected_R={1: 5}, expected_cycles=4)

# Two independent addi: last WB in cycle 5 = 5 cycles
check_test("timing_two_independent_addi",
    "addi R1, R0, 5\naddi R2, R0, 3",
    expected_R={1: 5, 2: 3}, expected_cycles=5)

# RAW chain: addi R2 waits for addi R1 broadcast (cycle 4, vj_ready=4),
# can start cycle 5, WB cycle 6
check_test("timing_raw_addi_chain",
    "addi R1, R0, 5\naddi R2, R1, 3",
    expected_R={1: 5, 2: 8}, expected_cycles=6)

# bne taken (R1=5, R0=0 → taken), skips addi R2 entirely.
# bne completes cycle 5, is_complete after cycle 5 → 5 cycles total
check_test("timing_branch_taken_only",
    "addi R1, R0, 5\nbne R1, R0, skip\naddi R2, R0, 9\nskip:",
    expected_R={1: 5, 2: 0}, expected_cycles=5)

# bne not taken (R1=0 == R0=0): bne resolves cycle 5 same as above,
# but then fetches + issues + executes addi R2 (4 more cycles: fetch=6,issue=7,start+finish=8,WB=9)
check_test("timing_branch_not_taken",
    "addi R1, R0, 0\nbne R1, R0, skip\naddi R2, R0, 9\nskip:",
    expected_R={1: 0, 2: 9}, expected_cycles=9)

# prog.dat — the official benchmark must be exactly 48 cycles
check_test("timing_prog_dat",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'prog.dat')).read(),
    expected_R={1: 0, 2: 100},
    expected_F={0: 195.0, 2: 12.0, 4: 27.0},
    expected_M={108: 195.0, 116: 63.0, 124: 128.0},
    expected_cycles=48)

# hazard_test.dat — CDB-priority / RAW test, confirmed 15 cycles
check_test("timing_hazard_test",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'hazard_test.dat')).read(),
    expected_R={1: 8, 2: 12, 3: 20},
    expected_F={0: 10.0, 2: 1.0, 4: 11.0},
    expected_M={16: 11.0},
    expected_cycles=15)

# branch_test.dat — 3-iteration loop, confirmed 36 cycles
check_test("timing_branch_test",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'branch_test.dat')).read(),
    expected_R={1: 24, 2: 48, 3: 0},
    expected_F={0: 6.0},
    expected_M={24: 2.0, 32: 4.0, 40: 6.0},
    expected_cycles=36)

# ─────────────────────────────────────────────────────────────────
# SECTION 2: R0 HARDWIRED TO ZERO
# ─────────────────────────────────────────────────────────────────
check_test("r0_write_ignored",
    "addi R0, R0, 42\naddi R1, R0, 1",
    expected_R={0: 0, 1: 1})

# $0 alias in bne must equal 0
check_test("r0_dollar_zero_alias",
    "addi R1, R0, 5\nbne R1, $0, skip\naddi R2, R0, 99\nskip: addi R3, R0, 77",
    expected_R={1: 5, 2: 0, 3: 77})

# ─────────────────────────────────────────────────────────────────
# SECTION 3: BASIC INTEGER ARITHMETIC
# ─────────────────────────────────────────────────────────────────
check_test("addi_positive",
    "addi R1, R0, 100",
    expected_R={1: 100})

check_test("addi_negative_immediate",
    "addi R1, R0, -7",
    expected_R={1: -7})

check_test("addi_chain_negative",
    "addi R1, R0, 10\naddi R1, R1, -3",
    expected_R={1: 7})

check_test("add_two_regs",
    "addi R1, R0, 10\naddi R2, R0, 20\nadd R3, R1, R2",
    expected_R={1: 10, 2: 20, 3: 30})

check_test("add_same_reg_twice",
    "addi R1, R0, 7\nadd R2, R1, R1",
    expected_R={1: 7, 2: 14})

check_test("32bit_signed_overflow",
    # 2147483647 + 1 wraps to -2147483648 in signed 32-bit
    "addi R1, R0, 2147483647\naddi R2, R0, 1\nadd R3, R1, R2",
    expected_R={3: -2147483648})

check_test("32bit_negative_addi_wrap",
    # addi with large negative: addi Rx, R0, -2147483648 = 0x80000000 = -2147483648
    "addi R1, R0, -2147483648",
    expected_R={1: -2147483648})

# ─────────────────────────────────────────────────────────────────
# SECTION 4: RAW DATA HAZARDS (forwarding via CDB)
# ─────────────────────────────────────────────────────────────────
check_test("raw_three_deep",
    "addi R1, R0, 1\nadd R2, R1, R1\nadd R3, R2, R2\nadd R4, R3, R3",
    expected_R={1: 1, 2: 2, 3: 4, 4: 8})

check_test("raw_long_chain",
    "addi R1, R0, 1\nadd R2, R1, R1\nadd R3, R2, R2\nadd R4, R3, R3\nadd R5, R4, R4\nadd R6, R5, R5",
    expected_R={1: 1, 2: 2, 3: 4, 4: 8, 5: 16, 6: 32})

check_test("raw_fan_out",
    # R1 feeds R2, R3, R4 — all must forward from CDB
    "addi R1, R0, 3\nadd R2, R1, R1\nadd R3, R1, R2\nadd R4, R1, R3",
    expected_R={1: 3, 2: 6, 3: 9, 4: 12})

# ─────────────────────────────────────────────────────────────────
# SECTION 5: WAW HAZARDS (register renaming via RS tags)
# ─────────────────────────────────────────────────────────────────
check_test("waw_basic",
    # I1 writes R1=5, I2 overwrites R1=10. Final R1 must be 10.
    "addi R1, R0, 5\naddi R1, R0, 10\naddi R2, R1, 1",
    expected_R={1: 10, 2: 11})

check_test("waw_read_after_second_write",
    # I3 must see I2's result (10), not I1's (5)
    "addi R1, R0, 5\naddi R1, R0, 10\nadd R2, R1, R1",
    expected_R={1: 10, 2: 20})

check_test("waw_fp_register",
    "0, 3.0\n8, 7.0\nfld F0, 0(R0)\nfld F0, 8(R0)\nfadd F1, F0, F0",
    expected_F={0: 7.0, 1: 14.0})

# ─────────────────────────────────────────────────────────────────
# SECTION 6: BRANCHES
# ─────────────────────────────────────────────────────────────────
check_test("bne_not_taken_continues",
    "addi R1, R0, 5\naddi R2, R0, 5\nbne R1, R2, skip\naddi R3, R0, 99\nskip: addi R4, R0, 77",
    expected_R={1: 5, 2: 5, 3: 99, 4: 77})

check_test("bne_taken_skips",
    "addi R1, R0, 5\naddi R2, R0, 6\nbne R1, R2, skip\naddi R3, R0, 99\nskip: addi R4, R0, 77",
    expected_R={1: 5, 2: 6, 3: 0, 4: 77})

check_test("bne_taken_backward_loop",
    # Accumulate: R2 = 3+2+1 = 6
    "addi R1, R0, 3\naddi R2, R0, 0\nloop: add R2, R2, R1\naddi R1, R1, -1\nbne R1, $0, loop",
    expected_R={1: 0, 2: 6})

check_test("bne_loop_5_iters",
    "addi R1, R0, 5\naddi R2, R0, 0\nloop: addi R2, R2, 1\naddi R1, R1, -1\nbne R1, $0, loop",
    expected_R={1: 0, 2: 5})

check_test("bne_never_taken",
    # bne R0, R0 → always not taken (0==0)
    "bne R0, R0, skip\naddi R1, R0, 42\nskip:",
    expected_R={1: 42})

check_test("bne_with_pending_operands",
    # Both operands come from in-flight instructions, branch must wait for CDB
    "addi R1, R0, 3\naddi R2, R0, 4\nbne R1, R2, done\naddi R3, R0, 0\ndone: addi R4, R0, 1",
    expected_R={1: 3, 2: 4, 3: 0, 4: 1})  # taken → R3 never runs

check_test("two_sequential_branches",
    "addi R1, R0, 1\naddi R2, R0, 2\nbne R1, R0, tgt1\naddi R5, R0, 0\ntgt1: bne R2, R0, tgt2\naddi R5, R0, 0\ntgt2: addi R3, R0, 99",
    expected_R={1: 1, 2: 2, 3: 99})

# ─────────────────────────────────────────────────────────────────
# SECTION 7: FLOATING-POINT OPERATIONS
# ─────────────────────────────────────────────────────────────────
check_test("fld_basic",
    "0, 42.0\nfld F1, 0(R0)",
    expected_F={1: 42.0}, expected_M={0: 42.0})

check_test("fld_nonzero_offset",
    "0, 1.0\n8, 2.0\n16, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfld F2, 16(R0)",
    expected_F={0: 1.0, 1: 2.0, 2: 3.0})

check_test("fld_with_pending_base",
    # Base address computed by addi, fld must wait
    "8, 99.0\naddi R1, R0, 8\nfld F0, 0(R1)",
    expected_F={0: 99.0}, expected_R={1: 8})

check_test("fsd_basic",
    "0, 0.0\nfld F0, 0(R0)\nfadd F0, F0, F0\nfsd F0, 0(R0)",
    expected_M={0: 0.0})  # 0+0=0 stored back

check_test("fsd_writes_correct_value",
    "0, 5.0\n8, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfadd F2, F0, F1\nfsd F2, 0(R0)",
    expected_F={0: 5.0, 1: 3.0, 2: 8.0}, expected_M={0: 8.0, 8: 3.0})

check_test("fadd_basic",
    "0, 3.0\n8, 4.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfadd F2, F0, F1",
    expected_F={0: 3.0, 1: 4.0, 2: 7.0})

check_test("fmul_basic",
    "0, 3.0\n8, 4.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfmul F2, F0, F1",
    expected_F={0: 3.0, 1: 4.0, 2: 12.0})

check_test("fdiv_basic",
    "0, 10.0\n8, 4.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfdiv F2, F0, F1",
    expected_F={0: 10.0, 1: 4.0, 2: 2.5})

check_test("fdiv_non_integer_result",
    "0, 1.0\n8, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfdiv F2, F0, F1",
    expected_F={2: 1.0 / 3.0})

check_test("float_chain_fmul_fadd",
    "0, 2.0\n8, 3.0\n16, 4.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfld F2, 16(R0)\nfmul F3, F0, F1\nfadd F4, F3, F2",
    # F3 = 2*3 = 6, F4 = 6+4 = 10
    expected_F={0: 2.0, 1: 3.0, 2: 4.0, 3: 6.0, 4: 10.0})

check_test("float_long_chain",
    "0, 2.0\nfld F0, 0(R0)\nfadd F1, F0, F0\nfmul F2, F1, F1\nfdiv F3, F2, F1",
    # F1=4, F2=16, F3=16/4=4
    expected_F={0: 2.0, 1: 4.0, 2: 16.0, 3: 4.0})

# ─────────────────────────────────────────────────────────────────
# SECTION 8: MEMORY ORDERING (store-before-load to same address)
# ─────────────────────────────────────────────────────────────────
check_test("store_then_load_same_addr",
    # fsd writes 8.0 to mem[0]; fld that follows must see 8.0, not the initial 1.0
    "0, 1.0\n8, 8.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfsd F1, 0(R0)\nfld F2, 0(R0)",
    expected_F={0: 1.0, 1: 8.0, 2: 8.0}, expected_M={0: 8.0})

check_test("load_then_store_war",
    # fld reads initial value; fsd writes new value after
    "0, 5.0\nfld F0, 0(R0)\nfadd F1, F0, F0\nfsd F1, 0(R0)",
    expected_F={0: 5.0, 1: 10.0}, expected_M={0: 10.0})

check_test("multiple_stores_then_load",
    "0, 1.0\n8, 2.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfsd F0, 16(R0)\nfsd F1, 16(R0)\nfld F2, 16(R0)",
    # Last store wins: F1=2.0 stored to mem[16]; F2 must be 2.0
    expected_F={2: 2.0}, expected_M={16: 2.0})

# ─────────────────────────────────────────────────────────────────
# SECTION 9: STRUCTURAL HAZARDS (full reservation stations)
# ─────────────────────────────────────────────────────────────────
# FPDIV RS size=1: second fdiv stalls until first fdiv's RS is freed (after WB).
# fdiv latency=6, so the second fdiv stalls for 8 cycles in the decode buffer.
check_test("structural_hazard_fpdiv_rs",
    "0, 6.0\n8, 2.0\n16, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfld F2, 16(R0)\nfdiv F3, F0, F1\nfdiv F4, F2, F1\naddi R1, R0, 1",
    expected_F={0: 6.0, 1: 2.0, 2: 3.0, 3: 3.0, 4: 1.5},
    expected_stalls=8)

# With default LS RS=3, 4 loads produce 0 stalls because WB frees a slot
# just before the 4th load needs to issue (single-issue + latency-2 pipelines).
# Force 1 stall by shrinking LS RS to 2: now 3 loads overwhelm the 2-slot RS.
check_test("structural_hazard_ls_rs",
    "0, 1.0\n8, 2.0\n16, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfld F2, 16(R0)",
    expected_F={0: 1.0, 1: 2.0, 2: 3.0},
    expected_stalls=1,
    extra_args=['--ls-rs', '2'])

# With default INT RS=4, 5 independent addi produce 0 stalls because each
# WBs the slot before the next instruction needs it (latency-1 pipeline).
# Force 1 stall by shrinking INT RS to 1: now 2 addi overwhelm the 1-slot RS.
check_test("structural_hazard_int_rs",
    "addi R1, R0, 1\naddi R2, R0, 2",
    expected_R={1: 1, 2: 2},
    expected_stalls=1,
    extra_args=['--int-rs', '1'])

# ─────────────────────────────────────────────────────────────────
# SECTION 10: CDB CONTENTION (multiple units finish same cycle)
# ─────────────────────────────────────────────────────────────────
# INT (latency 1) and FPADD (latency 3) stagger to finish same cycle.
# Key check: both results must be correct regardless of arbitration order.
check_test("cdb_contention_int_fpadd",
    "0, 5.0\n8, 3.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfadd F2, F0, F1\naddi R1, R0, 99\nfadd F3, F2, F2",
    expected_F={0: 5.0, 1: 3.0, 2: 8.0, 3: 16.0},
    expected_R={1: 99})

# Two loads issued back-to-back: second must wait for first (memory ordering).
# Then fadd depends on both — result must be correct.
check_test("cdb_two_loads_then_fadd",
    "0, 3.0\n8, 4.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfadd F2, F0, F1",
    expected_F={0: 3.0, 1: 4.0, 2: 7.0})

# ─────────────────────────────────────────────────────────────────
# SECTION 11: MIXED / COMPLEX PROGRAMS
# ─────────────────────────────────────────────────────────────────
check_test("mixed_int_and_float",
    "0, 10.0\n8, 2.0\nfld F0, 0(R0)\nfld F1, 8(R0)\nfmul F2, F0, F1\nfsd F2, 16(R0)\nfld F3, 16(R0)\nfadd F4, F3, F0\naddi R1, R0, 3\nadd R2, R1, R1",
    expected_F={0: 10.0, 1: 2.0, 2: 20.0, 3: 20.0, 4: 30.0},
    expected_R={1: 3, 2: 6},
    expected_M={16: 20.0})

check_test("address_from_addi_chain",
    # Base address is computed through a chain of addi instructions
    "0, 99.0\naddi R1, R0, 4\naddi R1, R1, -4\nfld F0, 0(R1)",
    expected_F={0: 99.0}, expected_R={1: 0})

check_test("all_eight_instructions",
    # Uses all 8 opcodes: fld fsd add addi fadd fmul fdiv bne
    "0, 6.0\n8, 3.0\n16, 0.0\naddi R1, R0, 8\nfld F0, 0(R0)\nfld F1, 0(R1)\nfadd F2, F0, F1\nfmul F3, F0, F1\nfdiv F4, F0, F1\nadd R2, R1, R1\nfsd F4, 16(R0)\nbne R0, R1, skip\naddi R5, R0, 42\nskip:",
    # F2=9, F3=18, F4=2, R2=16, mem[16]=2, R1!=0 so bne taken, R5=0
    expected_F={0: 6.0, 1: 3.0, 2: 9.0, 3: 18.0, 4: 2.0},
    expected_R={1: 8, 2: 16, 5: 0},
    expected_M={16: 2.0})

check_test("dot_product_3d",
    # Compute A·B where A=(2,3,4), B=(1,2,3)
    # Expected: 2*1 + 3*2 + 4*3 = 2 + 6 + 12 = 20
    "0, 2.0\n8, 3.0\n16, 4.0\n24, 1.0\n32, 2.0\n40, 3.0\nfld F0, 0(R0)\nfld F1, 24(R0)\nfmul F6, F0, F1\nfld F2, 8(R0)\nfld F3, 32(R0)\nfmul F7, F2, F3\nfld F4, 16(R0)\nfld F5, 40(R0)\nfmul F8, F4, F5\nfadd F9, F6, F7\nfadd F10, F9, F8",
    expected_F={6: 2.0, 7: 6.0, 8: 12.0, 9: 8.0, 10: 20.0})

check_test("fibonacci_first_10",
    # Compute fib(10) iteratively using integer regs
    # F(1)=1, F(2)=1, F(3)=2, ... F(10)=55
    # R1=prev, R2=curr, R3=counter (10 iterations), R4=temp
    """addi R1, R0, 0
addi R2, R0, 1
addi R3, R0, 9
fib: add R4, R1, R2
add R1, R0, R2
add R2, R0, R4
addi R3, R3, -1
bne R3, $0, fib""",
    expected_R={1: 34, 2: 55, 3: 0})

# ─────────────────────────────────────────────────────────────────
# SECTION 12: BONUS PARAMETERIZATION
# ─────────────────────────────────────────────────────────────────
# NI=4 should not change functional results, may change cycle count
check_test("bonus_ni4_same_result",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'prog.dat')).read(),
    expected_R={1: 0, 2: 100},
    expected_F={0: 195.0, 2: 12.0, 4: 27.0},
    expected_M={108: 195.0, 116: 63.0, 124: 128.0},
    extra_args=['--ni', '4'])

# NI=16 should also give same functional result
check_test("bonus_ni16_same_result",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'prog.dat')).read(),
    expected_R={1: 0, 2: 100},
    expected_F={0: 195.0, 2: 12.0, 4: 27.0},
    expected_M={108: 195.0, 116: 63.0, 124: 128.0},
    extra_args=['--ni', '16'])

# --all-rs 2: functional result must be the same, but more stalls expected
check_test("bonus_all_rs2_same_result",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'prog.dat')).read(),
    expected_R={1: 0, 2: 100},
    expected_F={0: 195.0, 2: 12.0, 4: 27.0},
    expected_M={108: 195.0, 116: 63.0, 124: 128.0},
    extra_args=['--all-rs', '2'])

# --all-rs 2 should give strictly more stalls than default for prog.dat
# (verified manually: default=6, all-rs-2=8)
check_test("bonus_all_rs2_more_stalls",
    open(os.path.join(os.path.dirname(__file__), 'benchmarks', 'prog.dat')).read(),
    expected_stalls=8,
    extra_args=['--all-rs', '2'])

# ─────────────────────────────────────────────────────────────────
# SUMMARY
# ─────────────────────────────────────────────────────────────────
total = len(passed) + len(failed)
print(f"\n{'='*60}")
print(f"Results: {len(passed)}/{total} passed")
if failed:
    print(f"\nFailed tests:")
    for t in failed:
        print(f"  - {t}")
else:
    print("All tests passed!")
