# CS 162 - Tomasulo's Algorithm Simulator

## Overview
A Python implementation of a Tomasulo's Algorithm simulator for a non-speculative, single-issue, out-of-order CPU (based on simplified PowerPC 604/620 architecture). Supports 8 RISC-V instructions: `fld`, `fsd`, `add`, `addi`, `fadd`, `fmul`, `fdiv`, `bne`.

## Requirements
- Python 3.6+
- No external libraries required

## Compilation / Setup
No compilation needed. Run directly with Python 3.

## How to Run

```bash
python3 tomasulo_simulator.py <input_file> [NI] [RS_SIZE]
```

### Arguments
| Argument | Description | Default |
|----------|-------------|---------|
| `input_file` | Path to the RISC-V assembly input file | (required) |
| `NI` | Decode buffer (instruction queue) size | 1 |
| `RS_SIZE` | Override reservation station count for ALL functional units | (per-FU defaults) |

### Examples

```bash
# Run with default parameters
python3 tomasulo_simulator.py prog.dat

# Bonus: Run with NI=4 (larger decode buffer)
python3 tomasulo_simulator.py prog.dat 4

# Bonus: Run with NI=16
python3 tomasulo_simulator.py prog.dat 16

# Bonus: Run with all RS sizes = 2
python3 tomasulo_simulator.py prog.dat 1 2
```

## Input File Format

```
%comment lines start with % or #
%memory initialization: address, value
0, 111
8, 14
...

% Instructions follow (labels optional)
addi R1, R0, 24
loop: fld F0, 0(R1)
bne R1, R0, loop
```

## Architecture Parameters (default)

| Unit      | Latency | RS Count | Instructions      |
|-----------|---------|----------|-------------------|
| INT       | 1       | 4        | add, addi         |
| LoadStore | 2       | 3        | fld, fsd          |
| FPadd     | 3       | 3        | fadd              |
| FPmult    | 4       | 2        | fmul              |
| FPdiv     | 6       | 1        | fdiv              |
| BU        | 1       | 1        | bne               |

- NF=1 (fetch width), NI=1 (decode buffer), NW=1 (issue width), NB=1 (CDB count)
- No ROB, no branch prediction
- CDB arbitration: oldest instruction (by issue order) wins
