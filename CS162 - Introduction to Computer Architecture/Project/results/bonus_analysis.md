# Bonus Analysis

The simulator supports both requested bonus controls directly from the command line:

- `--ni N`
- `--all-rs N` or per-unit overrides such as `--int-rs`, `--ls-rs`, and `--fpadd-rs`

## Commands Used

```bash
python3 simulator.py benchmarks/prog.dat
python3 simulator.py benchmarks/prog.dat --ni 4
python3 simulator.py benchmarks/prog.dat --ni 16
python3 simulator.py benchmarks/prog.dat --all-rs 2
```

## Measured Results

| Configuration | Cycles | Issue Stalls |
| --- | ---: | ---: |
| Default (`NI=1`, default RS sizes) | 48 | 6 |
| `NI=4`, default RS sizes | 48 | 6 |
| `NI=16`, default RS sizes | 48 | 6 |
| `NI=1`, all RS sizes = 2 | 48 | 8 |

## Interpretation

- Increasing `NI` from 1 to 4 or 16 did not reduce runtime for the provided benchmark.
- The reason is that the benchmark is constrained primarily by loop-carried data dependences, branch fetch stalls, single issue, and the single CDB.
- Forcing all reservation-station banks to size 2 increased the number of issue stalls from 6 to 8.
- Even with extra issue pressure, the total cycle count remained 48 because the additional stalls did not extend the benchmark's critical path.

## Ready-to-Paste Report Subsection

### Bonus Parameterization Study

The simulator was extended to support parameterized decode-buffer capacity (`NI`) and configurable reservation-station counts for each functional-unit class. These parameters are exposed through command-line options, allowing the same simulator binary to be reused for comparative studies without code changes.

For the provided benchmark, increasing `NI` from 1 to 4 and 16 had no effect on the total execution time: all three runs completed in 48 cycles and recorded 6 issue-stall events. This indicates that the benchmark is not decode-buffer limited. Instead, it is constrained by true dependences inside the loop, serialized branch fetch behavior, single-issue dispatch, and single-CDB writeback.

When all reservation-station banks were forced to size 2, the simulator still completed the benchmark in 48 cycles, but issue-stall events increased from 6 to 8. This shows that reducing RS capacity does create additional structural pressure at issue, but for this workload the extra pressure does not move the overall critical path.

## Relevant Implementation Hooks

The bonus support is implemented in [simulator.py](/Users/kaushikvada/Documents/Project/simulator.py) through:

- `SimulatorConfig`, which stores `ni` and per-unit `rs_sizes`
- `build_arg_parser()`, which exposes `--ni`, `--all-rs`, and per-unit RS options
- `make_config()`, which assembles the final configuration from CLI arguments
- `TomasuloSimulator.__init__()`, which sizes the decode queue and RS banks from the configuration
