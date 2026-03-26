# Tomasulo CPU Simulator

CS 162 project -- cycle-accurate simulator for an out-of-order processor using Tomasulo's algorithm (no ROB). Tracks reservation stations, functional unit latencies, register renaming via tags, CDB arbitration, and branch stalls.

## Files

```text
.
├── simulation.py       # main simulator
├── README.md
├── report.pdf
├── report.tex          # LaTeX source for report
└── benchmarks/
    └── prog.dat        # provided benchmark
```

## What it models

- 32 integer registers (R0-R31) and 32 FP registers (F0-F31), 32-bit signed
- Single fetch and single issue per cycle
- One CDB -- only one broadcast per cycle
- Non-speculative: stalls on every branch until it resolves
- No ROB -- registers update on CDB broadcast
- Functional units (from the spec):

| Unit  | Latency | RS slots | Ops          |
|-------|---------|----------|--------------|
| INT   | 1       | 4        | add, addi    |
| LS    | 2       | 3        | fld, fsd     |
| FPADD | 3       | 3        | fadd         |
| FPMUL | 4       | 2        | fmul         |
| FPDIV | 6       | 1        | fdiv         |
| BU    | 1       | 1        | bne          |

## Requirements

Python 3.10+, no external libraries.

## How to run

```bash
python3 simulation.py benchmarks/prog.dat
```

More examples:

```bash
python3 simulation.py benchmarks/hazard_test.dat
python3 simulation.py benchmarks/branch_test.dat
python3 simulation.py benchmarks/prog.dat --trace
python3 simulation.py benchmarks/prog.dat --ni 4
python3 simulation.py benchmarks/prog.dat --all-rs 2
python3 simulation.py benchmarks/prog.dat --ni 16 --json-out results/run.json
```

## Flags

| Flag | What it does |
|------|-------------|
| `--ni N` | decode buffer size (default 1) |
| `--all-rs N` | set all RS banks to size N |
| `--int-rs N` | override INT RS size |
| `--ls-rs N` | override LS RS size |
| `--fpadd-rs N` | override FPADD RS size |
| `--fpmul-rs N` | override FPMUL RS size |
| `--fpdiv-rs N` | override FPDIV RS size |
| `--bu-rs N` | override BU RS size |
| `--trace` | print per-cycle trace |
| `--json-out PATH` | dump results to JSON |

## Input format

Memory init lines (address, value) come before the instructions:

```
108, 27
116, 3
124, 8
```

Supported instructions:

```
fld Fd, offset(Rb)
fsd Fs, offset(Rb)
add Rd, Rs1, Rs2
addi Rd, Rs1, imm
fadd Fd, Fs1, Fs2
fmul Fd, Fs1, Fs2
fdiv Fd, Fs1, Fs2
bne Rs1, Rs2, label
```

Labels like `loop:` can be on their own line or inline before an instruction.

## Output

Prints cycle count, stall count, final integer registers, final FP registers, and any modified memory addresses. Sample output files are in `results/`.

## Timing rules

- fetch in cycle N → earliest issue is N+1
- issue in cycle N → earliest execution start is N+1
- execution done in cycle N → CDB eligible starting N+1
- CDB captured in cycle N → earliest execution start is N+1
- branch resolves in cycle N → fetch resumes N+1
- R0 is hardwired to 0, writes are ignored
- uninitialized memory reads return 0.0
- memory ops run in issue order (no disambiguation)

## Bonus

NI and all RS sizes are configurable from the command line. Comparison results are in `results/bonus_analysis.md`.

## Limitations

- only the 8 required opcodes
- no speculative execution or branch prediction
- no store-to-load forwarding
- division by zero exits with an error
