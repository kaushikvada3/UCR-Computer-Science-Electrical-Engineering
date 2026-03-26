# Project Report: Tomasulo Functional and Timing Simulator

## 1. Introduction

This project implements a functional plus timing simulator for a non-speculative, single-issue, out-of-order processor based on Tomasulo's Algorithm without a reorder buffer. The simulator accepts a small RISC-V-like assembly subset, models reservation stations, functional-unit latencies, register renaming through tags, CDB arbitration, branch stalls, and produces both final architectural state and timing statistics.

The primary project goals were:

- preserve functional correctness for registers and memory
- model execution timing cycle by cycle
- handle out-of-order execution through reservation stations and tag-based operand tracking
- enforce the specified structural limits such as single issue, single CDB, and single branch resolution path

## 2. Architecture Modeled

The simulated machine is a 32-bit architecture with separate instruction and data memories. The supported instruction subset is:

- `fld`
- `fsd`
- `add`
- `addi`
- `fadd`
- `fmul`
- `fdiv`
- `bne`

The machine uses the following functional units and reservation-station counts by default:

| Unit | Latency | Reservation Stations | Supported Ops |
| --- | ---: | ---: | --- |
| INT | 1 | 4 | `add`, `addi` |
| LS | 2 | 3 | `fld`, `fsd` |
| FPADD | 3 | 3 | `fadd` |
| FPMUL | 4 | 2 | `fmul` |
| FPDIV | 6 | 1 | `fdiv` |
| BU | 1 | 1 | `bne` |

The fetch width is one instruction per cycle, issue width is one instruction per cycle, and there is one common data bus. Branches are non-speculative: fetch stalls as soon as a branch is fetched and remains stalled until the branch finishes in the branch unit.

## 3. Simulator Organization

The implementation is intentionally compact and grading-friendly. The main modules inside the single-file implementation are:

- `ProgramParser`: parses memory initialization, labels, and instructions
- `Instruction`: immutable parsed instruction representation
- `ReservationStationEntry`: dynamic state for a reservation station slot
- `FunctionalUnitState`: execution state for one physical functional unit of a given class
- `TomasuloSimulator`: core cycle-by-cycle simulator
- CLI and formatting helpers: argument parsing, textual output, and optional JSON summaries

This structure separates static program description from dynamic execution state. The parser resolves labels once, and the simulator then operates on resolved instruction objects cycle by cycle.

## 4. Key Data Structures

The simulator uses the following data structures.

### 4.1 Architectural Register Files

- Integer register file: dictionary keyed by `R0` to `R31`
- Floating-point register file: dictionary keyed by `F0` to `F31`

Integer results are wrapped to signed 32-bit values. `R0` is hardwired to zero.

### 4.2 Register Status Tables

Separate status tables are maintained for integer and floating-point registers. Each entry stores either:

- `None`, meaning the architectural register currently holds the valid value, or
- a reservation-station tag, meaning the value is pending from that producer

This directly implements Tomasulo-style implicit register renaming.

### 4.3 Memory Model

Data memory is implemented as a sparse dictionary from byte addresses to floating-point values. Unspecified addresses read as `0.0`. Instruction memory is a separate list of parsed instructions, and the PC indexes that list rather than byte addresses.

### 4.4 Instruction Representation

Each parsed instruction stores:

- opcode
- static PC
- original text
- source line number
- destination register when applicable
- source registers
- base register and offset for memory operations
- resolved target PC for branches

### 4.5 Reservation Stations

Each reservation-station entry stores:

- unique tag such as `INT1` or `A2`
- busy bit
- opcode and pointer to the instruction
- issue order number
- issue cycle
- destination architectural register, if any
- operand values `Vj` and `Vk`
- operand producer tags `Qj` and `Qk`
- operand ready cycles
- execution flags and remaining latency
- completion cycle
- result value for CDB-producing instructions
- memory-phase completion flag for load/store ordering

### 4.6 Decode Buffer / Instruction Queue

The decode queue is implemented as a FIFO deque with capacity `NI`. It behaves as a simple latch for `NI = 1` and as a shallow instruction queue for larger `NI`.

### 4.7 Functional Units

Each functional-unit class has exactly one executing pipeline instance. A `FunctionalUnitState` records:

- which reservation-station tag is currently executing
- cycles remaining for that execution

### 4.8 Statistics

The simulator records:

- total execution cycles
- issue-stage stall events caused by a full reservation-station bank

## 5. Pipeline and Cycle Behavior

The most important implementation choice in the project is the cycle ordering. Timing errors usually appear when issue, execution, writeback, and wakeup are allowed to overlap too aggressively in a single cycle. To avoid this, the simulator uses a strict five-step cycle model:

1. **Writeback / CDB arbitration**  
   At most one completed instruction may broadcast in a cycle. Only instructions that completed in an earlier cycle are eligible. This prevents same-cycle finish-and-broadcast behavior.

2. **Issue**  
   The oldest instruction in the decode queue issues if a matching reservation-station slot is free. If not, an issue-stall event is counted.

3. **Fetch**  
   One instruction is fetched if the decode queue has space and no unresolved branch is blocking fetch. If the fetched instruction is `bne`, fetch stall is asserted immediately.

4. **Execution start**  
   Idle functional units select the oldest ready reservation-station entry of their class. An entry is ready only if:
   - it issued in an earlier cycle
   - all required operands are present
   - each required operand became available in an earlier cycle
   - for memory operations, no older memory operation still has an unfinished memory phase

5. **Execution advance / completion**  
   All busy functional units advance by one cycle. When a unit reaches zero remaining cycles:
   - arithmetic and load instructions enter the completed-waiting-for-CDB state
   - stores update memory immediately and free their reservation station
   - branches resolve immediately, update PC, clear the branch fetch stall, and free their reservation station

This ordering guarantees that:

- issue and execution do not collapse into the same cycle
- wakeup from the CDB cannot start execution in the same cycle
- branch resolution changes the fetch PC for the next cycle, not the current one
- CDB contention is handled only among already completed instructions

## 6. Hazard Handling

### 6.1 RAW Hazards

RAW hazards are handled through register tags. When a source register is not yet ready, the issuing instruction stores the producer reservation-station tag in `Qj` or `Qk`. When the producer broadcasts, all listening reservation stations capture the value in the same cycle and become eligible to execute in the next cycle.

### 6.2 WAW Hazards

WAW hazards are handled naturally by the register status table. A younger writer simply overwrites the destination register's status entry with its own tag. When an older writer later broadcasts, it updates only waiting stations and the architectural register file only if that register still points to the older tag.

### 6.3 WAR Hazards

WAR hazards are eliminated by reservation-station renaming. Once an instruction issues, it either captures the source value or records the producer tag, so later destination writes cannot destroy the needed operand.

### 6.4 Structural Hazards

The simulator enforces:

- one fetch per cycle
- one issue per cycle
- one execution pipeline per functional-unit class
- one CDB broadcast per cycle
- one load/store execution pipeline, which also satisfies the single-ported memory requirement

Issue-stage structural stalls are counted only when the matching reservation-station bank is full.

## 7. Branch Handling

Branches are handled conservatively because the machine is non-speculative. When `bne` is fetched, fetch immediately stalls. No younger instruction is fetched until the branch executes in the branch unit and the correct next PC is known.

The branch itself may wait in the decode queue or reservation station while older instructions continue to execute. Once the branch finishes:

- if taken, the PC is set to the label target
- if not taken, the PC is set to `branch_pc + 1`
- fetch restarts in the following cycle

Because no younger instructions are ever fetched past the unresolved branch, the simulator never needs a flush mechanism.

## 8. CDB Arbitration Policy

The CDB arbitration rule is:

- only one result-producing instruction may broadcast per cycle
- only instructions completed in an earlier cycle may compete
- the winner is the oldest completed instruction by issue order

This policy is especially important when different functional units complete in the same cycle. A focused hazard benchmark was included to validate this behavior. In that benchmark, an older `fld` and a younger `addi` finish in the same cycle, and the load correctly receives the CDB first. The dependent second load therefore waits one extra cycle for `R1`, which matches the intended policy.

## 9. Benchmark Methodology

Three benchmark programs were used.

1. `prog.dat`: the course-provided loop benchmark
2. `hazard_test.dat`: validates CDB priority, integer RAW chains, and FP load/use behavior
3. `branch_test.dat`: validates fetch stall on branch and repeated loop execution

Each benchmark was run with the default configuration. The required bonus comparisons were then run on `prog.dat` using:

- `NI = 4`
- `NI = 16`
- all reservation-station banks forced to size 2

## 10. Results for the Provided Benchmark

The provided benchmark produces the following measured result under the default configuration:

- Total cycles: `48`
- Issue stall events: `6`

Final modified memory values are:

- `Mem[124] = 128.0`
- `Mem[116] = 63.0`
- `Mem[108] = 195.0`

These values match the expected loop behavior, because each iteration computes:

- `memory[R2] = memory[R1] * 12 + memory[R2]`
- Iteration 1: `10 * 12 + 8 = 128`
- Iteration 2: `5 * 12 + 3 = 63`
- Iteration 3: `14 * 12 + 27 = 195`

The final integer state shows `R1 = 0` and `R2 = 100`, which is also consistent with three iterations of decrements by 8.

### Bonus comparison results

| Configuration | Cycles | Issue Stalls | Observation |
| --- | ---: | ---: | --- |
| Default (`NI=1`, default RS) | 48 | 6 | Baseline |
| `NI=4` | 48 | 6 | No change |
| `NI=16` | 48 | 6 | No change |
| All RS sizes = 2 | 48 | 8 | Same cycles, more issue pressure |

For this benchmark, increasing `NI` does not improve cycles because the critical path is dominated by true dependences, branch fetch stalls, and the single issue / single CDB restrictions. Reducing reservation-station counts to 2 increases issue stalls but still does not lengthen the overall critical path for this particular loop.

## 11. Discussion Peers Section

No formal peer collaboration was used in preparing this implementation. As a result, there was no peer-derived design change to summarize in the final version of the simulator.

## 12. Challenges and Debugging Notes

The main implementation challenges were timing consistency and memory ordering.

- The first common source of error in Tomasulo simulators is allowing a result to complete, broadcast, wake up dependents, and start a dependent execution all in one cycle. The simulator avoids this by recording the cycle in which each operand became ready and requiring that execution begin strictly later.
- The second challenge is CDB contention. The solution was to keep completed instructions resident in their reservation stations until they actually win the bus, and to pick the winner by issue order.
- The third challenge is memory correctness without a reorder buffer or explicit disambiguation. To remain conservative and correct, the simulator blocks a younger memory operation from starting if an older memory operation still has an unfinished memory phase.
- Branch timing also required care. Fetch is stalled immediately when a branch is fetched, and the resolved PC becomes visible only in the next cycle after the branch unit finishes.

The cycle trace mode and the focused hazard benchmark were useful for debugging these cases.

## 13. Conclusion

The finished simulator satisfies the required instruction subset, resource constraints, branch behavior, CDB arbitration policy, and statistics reporting. It produces correct final register and memory state for the tested programs and uses a cycle model that is internally consistent and easy to justify.

The bonus parameterization was also implemented, allowing the decode queue size and each reservation-station bank size to be varied directly from the command line. The measured comparisons show that the provided benchmark is limited more by data dependences and branch serialization than by decode-buffer capacity.
