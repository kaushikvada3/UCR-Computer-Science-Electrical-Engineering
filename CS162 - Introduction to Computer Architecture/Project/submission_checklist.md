# Submission Checklist

## Deliverables

- [x] Full working simulator source code: [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py)
- [x] Clean project structure: [README.md](/Users/kaushikvada/Documents/Project/README.md)
- [x] README with compile and run instructions: [README.md](/Users/kaushikvada/Documents/Project/README.md)
- [x] Project report in markdown: [report.md](/Users/kaushikvada/Documents/Project/report.md)
- [x] Project report in LaTeX: [report.tex](/Users/kaushikvada/Documents/Project/report.tex)
- [x] Compiled report PDF: [report.pdf](/Users/kaushikvada/Documents/Project/report.pdf)
- [x] Benchmark input files: [benchmarks/prog.dat](/Users/kaushikvada/Documents/Project/benchmarks/prog.dat), [benchmarks/hazard_test.dat](/Users/kaushikvada/Documents/Project/benchmarks/hazard_test.dat), [benchmarks/branch_test.dat](/Users/kaushikvada/Documents/Project/benchmarks/branch_test.dat)
- [x] Example output format: [results/example_output_format.txt](/Users/kaushikvada/Documents/Project/results/example_output_format.txt)
- [x] Bonus parameterization support and analysis scaffolding: [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), [results/bonus_analysis.md](/Users/kaushikvada/Documents/Project/results/bonus_analysis.md)

## Requirement Cross-Map

| Requirement | Where it is addressed |
| --- | --- |
| Parse assembly program and memory initialization | `ProgramParser` in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py) |
| Support `fld`, `fsd`, `add`, `addi`, `fadd`, `fmul`, `fdiv`, `bne` | opcode parsing and execution logic in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py) |
| 32-bit architecture and `R0` hardwired to zero | integer register handling in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py) and discussion in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Single-issue, out-of-order Tomasulo without ROB | simulator core in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), design sections in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Decode buffer / instruction queue with default `NI=1` | [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), README parameter section in [README.md](/Users/kaushikvada/Documents/Project/README.md) |
| Branch fetch stall until branch completion | branch handling in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), sections 5 and 7 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Functional-unit latencies and reservation stations | constants and RS-bank construction in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), architecture section in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Single CDB with oldest-by-issue priority | writeback arbitration in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), section 8 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Register renaming using RS tags | register status tables and issue logic in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), sections 4 and 6 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| CDB forwarding / wakeup | broadcast logic in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), section 6 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Separate instruction and data memories | parser plus simulator state in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), section 2 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |
| Total cycles and issue-stage stall statistics | [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), output examples in [results/prog_default.txt](/Users/kaushikvada/Documents/Project/results/prog_default.txt) |
| Final register and memory contents | printed output in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), sample outputs in [results/prog_default.txt](/Users/kaushikvada/Documents/Project/results/prog_default.txt) |
| README with compile and run instructions | [README.md](/Users/kaushikvada/Documents/Project/README.md) |
| Report with module design, data structures, results, and discussion peers | [report.md](/Users/kaushikvada/Documents/Project/report.md), [report.tex](/Users/kaushikvada/Documents/Project/report.tex) |
| Bonus parameterized `NI` and RS sizes | CLI and config logic in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py), [results/bonus_analysis.md](/Users/kaushikvada/Documents/Project/results/bonus_analysis.md) |
| Comparative analysis for `NI=4`, `NI=16`, and all RS sizes = 2 | [results/bonus_analysis.md](/Users/kaushikvada/Documents/Project/results/bonus_analysis.md), section 10 in [report.md](/Users/kaushikvada/Documents/Project/report.md) |

## Verified Execution Artifacts

- [results/prog_default.txt](/Users/kaushikvada/Documents/Project/results/prog_default.txt)
- [results/prog_ni4.txt](/Users/kaushikvada/Documents/Project/results/prog_ni4.txt)
- [results/prog_ni16.txt](/Users/kaushikvada/Documents/Project/results/prog_ni16.txt)
- [results/prog_all_rs_2.txt](/Users/kaushikvada/Documents/Project/results/prog_all_rs_2.txt)
- [results/hazard_test.txt](/Users/kaushikvada/Documents/Project/results/hazard_test.txt)
- [results/branch_test.txt](/Users/kaushikvada/Documents/Project/results/branch_test.txt)
