#!/usr/bin/env python3
"""
CS 162 Computer Architecture - Tomasulo's Algorithm Simulator
Simulates a modified PowerPC 604/620 architecture with:
- Non-speculative, single-issue, out-of-order CPU
- Tomasulo's Algorithm WITHOUT a Reorder Buffer
- Single Common Data Bus (CDB)
- Implicit register renaming via Reservation Station IDs
"""

import sys
import re
from copy import deepcopy

# ──────────────────────────────────────────────
# CONFIGURATION (parameterized for bonus part)
# ──────────────────────────────────────────────
NF = 1   # Instructions fetched per cycle
NI = 1   # Decode buffer size (instruction queue)
NW = 1   # Instructions issued per cycle
NB = 1   # Number of CDBs

# Functional unit definitions: name -> (latency, num_RS, [opcodes])
FU_CONFIG = {
    "INT":       (1, 4, ["add", "addi"]),
    "LoadStore": (2, 3, ["fld", "fsd"]),
    "FPadd":     (3, 3, ["fadd"]),
    "FPmult":    (4, 2, ["fmul"]),
    "FPdiv":     (6, 1, ["fdiv"]),
    "BU":        (1, 1, ["bne"]),
}


# ──────────────────────────────────────────────
# DATA STRUCTURES
# ──────────────────────────────────────────────

class ReservationStation:
    """Single reservation station entry."""
    def __init__(self, rs_id, fu_name, latency):
        self.rs_id = rs_id          # Unique RS identifier (e.g., "INT_0")
        self.fu_name = fu_name      # Which functional unit this belongs to
        self.latency = latency      # FU latency in cycles
        self.reset()

    def reset(self):
        self.busy = False
        self.op = None
        self.vj = None              # Value of operand 1 (None if not ready)
        self.vk = None              # Value of operand 2
        self.qj = None              # Tag for operand 1 (RS ID producing it)
        self.qk = None              # Tag for operand 2
        self.dest_reg = None        # Destination register name (e.g., "F0")
        self.imm = None             # Immediate value or address offset
        self.issue_cycle = None     # Cycle when instruction was issued
        self.exec_start = None      # Cycle when execution started
        self.exec_remaining = None  # Cycles remaining in execution
        self.result = None          # Computed result
        self.done = False           # Execution complete, waiting for CDB
        self.instr_index = None     # Original instruction index (for CDB priority)
        self.addr = None            # Memory address (for load/store)

    def ready(self):
        """True if both operands are available."""
        return self.busy and self.qj is None and self.qk is None and not self.done


class RegisterFile:
    """
    Holds values for integer registers R0-R31 and FP registers F0-F31.
    All initialized to 0.
    """
    def __init__(self):
        self.regs = {}
        for i in range(32):
            self.regs[f"R{i}"] = 0
            self.regs[f"F{i}"] = 0.0


class RegisterStatusTable:
    """
    RAT: tracks which RS (if any) is producing the next value for each register.
    None means the register file has the current valid value.
    """
    def __init__(self):
        self.status = {}
        for i in range(32):
            self.status[f"R{i}"] = None
            self.status[f"F{i}"] = None

    def get(self, reg):
        return self.status.get(reg, None)

    def set(self, reg, rs_id):
        self.status[reg] = rs_id

    def clear(self, reg, rs_id):
        """Only clear if this RS is still the producer (handles WAW)."""
        if self.status.get(reg) == rs_id:
            self.status[reg] = None


class Memory:
    """Simple data memory initialized from input file."""
    def __init__(self):
        self.mem = {}

    def read(self, addr):
        return self.mem.get(addr, 0)

    def write(self, addr, value):
        self.mem[addr] = value


# ──────────────────────────────────────────────
# INSTRUCTION PARSING
# ──────────────────────────────────────────────

def parse_register(s):
    """Normalize register name: r1 -> R1, f2 -> F2, $0 -> R0."""
    s = s.strip()
    if s == "$0" or s == "$zero":
        return "R0"
    if s.lower().startswith("r"):
        return "R" + s[1:]
    if s.lower().startswith("f"):
        return "F" + s[1:]
    return s

def parse_instruction(line, pc):
    """
    Parse a single instruction line. Returns dict with fields:
    op, rd, rs1, rs2, imm, label, pc
    """
    # Remove inline comments
    line = line.split('#')[0].split('%')[0].strip()
    if not line:
        return None

    # Remove label prefix (e.g., "loop:")
    label = None
    if ':' in line:
        parts = line.split(':', 1)
        # Check if it's a label (no spaces before colon)
        if ' ' not in parts[0]:
            label = parts[0].strip()
            line = parts[1].strip()
        else:
            # Could be something like "fld F0, 0(R1)" - colon not a label
            pass

    if not line:
        return None

    tokens = re.split(r'[\s,]+', line)
    tokens = [t for t in tokens if t]
    if not tokens:
        return None

    op = tokens[0].lower()
    instr = {"op": op, "pc": pc, "label": label, "rd": None, "rs1": None,
             "rs2": None, "imm": None, "mem_reg": None}

    try:
        if op == "addi":
            # addi rd, rs1, imm
            instr["rd"] = parse_register(tokens[1])
            instr["rs1"] = parse_register(tokens[2])
            instr["imm"] = int(tokens[3])

        elif op == "add":
            # add rd, rs1, rs2
            instr["rd"] = parse_register(tokens[1])
            instr["rs1"] = parse_register(tokens[2])
            instr["rs2"] = parse_register(tokens[3])

        elif op == "fld":
            # fld fd, offset(rs1)
            instr["rd"] = parse_register(tokens[1])
            # Parse offset(reg)
            m = re.match(r'(-?\d+)\((\w+)\)', tokens[2])
            if m:
                instr["imm"] = int(m.group(1))
                instr["mem_reg"] = parse_register(m.group(2))
            else:
                instr["imm"] = int(tokens[2])
                instr["mem_reg"] = parse_register(tokens[3])

        elif op == "fsd":
            # fsd fs, offset(rs1)
            instr["rs1"] = parse_register(tokens[1])   # source (data)
            m = re.match(r'(-?\d+)\((\w+)\)', tokens[2])
            if m:
                instr["imm"] = int(m.group(1))
                instr["mem_reg"] = parse_register(m.group(2))
            else:
                instr["imm"] = int(tokens[2])
                instr["mem_reg"] = parse_register(tokens[3])

        elif op == "fadd":
            # fadd fd, fs1, fs2
            instr["rd"] = parse_register(tokens[1])
            instr["rs1"] = parse_register(tokens[2])
            instr["rs2"] = parse_register(tokens[3])

        elif op == "fmul":
            instr["rd"] = parse_register(tokens[1])
            instr["rs1"] = parse_register(tokens[2])
            instr["rs2"] = parse_register(tokens[3])

        elif op == "fdiv":
            instr["rd"] = parse_register(tokens[1])
            instr["rs1"] = parse_register(tokens[2])
            instr["rs2"] = parse_register(tokens[3])

        elif op == "bne":
            # bne rs1, rs2, label
            instr["rs1"] = parse_register(tokens[1])
            instr["rs2"] = parse_register(tokens[2])
            instr["target_label"] = tokens[3]

        else:
            print(f"[WARN] Unknown opcode: {op}")
            return None

    except (IndexError, ValueError) as e:
        print(f"[ERROR] Failed to parse instruction: '{line}' -> {e}")
        return None

    return instr


def parse_input_file(filename):
    """
    Parse the input file. Returns:
    - instructions: list of parsed instruction dicts
    - memory: Memory object initialized with values
    - label_map: dict mapping label -> PC (instruction index)
    """
    memory = Memory()
    raw_instructions = []
    label_map = {}

    with open(filename, 'r') as f:
        lines = f.readlines()

    parsing_memory = True

    for line in lines:
        # Strip comments
        clean = line.split('%')[0].split('#')[0].strip()
        if not clean:
            continue

        # Memory initialization lines: "address, value"
        if parsing_memory and re.match(r'^-?\d+\s*,\s*-?\d+(\.\d+)?$', clean):
            parts = clean.split(',')
            addr = int(parts[0].strip())
            val_str = parts[1].strip()
            val = float(val_str) if '.' in val_str else int(val_str)
            memory.mem[addr] = val
            continue
        else:
            parsing_memory = False

        raw_instructions.append(clean)

    # First pass: collect labels and assign PCs
    instructions = []
    pc = 0
    pending_label = None

    for raw in raw_instructions:
        # Check for label prefix
        if ':' in raw:
            colon_idx = raw.index(':')
            potential_label = raw[:colon_idx].strip()
            if ' ' not in potential_label and '\t' not in potential_label:
                pending_label = potential_label
                rest = raw[colon_idx+1:].strip()
                if not rest:
                    continue
                raw = rest

        instr = parse_instruction(raw, pc)
        if instr is None:
            continue

        if pending_label:
            label_map[pending_label] = pc
            instr["label"] = pending_label
            pending_label = None

        instructions.append(instr)
        pc += 1

    return instructions, memory, label_map


# ──────────────────────────────────────────────
# SIMULATOR
# ──────────────────────────────────────────────

class TomasuloSimulator:
    def __init__(self, instructions, memory, label_map,
                 NI=1, fu_config=None):
        self.instructions = instructions
        self.memory = memory
        self.label_map = label_map
        self.NI = NI

        # Build functional unit configs
        self.fu_config = fu_config or FU_CONFIG

        # Build opcode -> FU name map
        self.op_to_fu = {}
        for fu_name, (lat, rs_count, ops) in self.fu_config.items():
            for op in ops:
                self.op_to_fu[op] = fu_name

        # Build reservation stations
        self.rs_list = []           # All RS objects in a flat list
        self.fu_rs = {}             # fu_name -> list of RS objects
        rs_id_counter = 0
        for fu_name, (lat, rs_count, ops) in self.fu_config.items():
            self.fu_rs[fu_name] = []
            for i in range(rs_count):
                rs = ReservationStation(rs_id_counter, fu_name, lat)
                self.rs_list.append(rs)
                self.fu_rs[fu_name].append(rs)
                rs_id_counter += 1

        # Register file and RAT
        self.reg_file = RegisterFile()
        self.rat = RegisterStatusTable()

        # Pipeline state
        self.pc = 0                  # Next instruction to fetch
        self.cycle = 0

        # Decode buffer (between Fetch and Issue)
        self.decode_buffer = []      # List of instructions waiting to be issued

        # Fetch stall (due to branch)
        self.fetch_stalled_for_branch = False
        self.branch_rs = None        # The RS executing the branch

        # Statistics
        self.total_cycles = 0
        self.structural_stalls = 0   # Times issue stalled due to full RS

        # Tracking
        self.issued_count = 0        # Total instructions issued (for CDB priority)
        self.completed_instructions = 0
        self.total_instructions = len(instructions)

        # For detecting completion
        self.all_fetched = False

    def get_reg_value(self, reg):
        """Get current register value from register file. R0 is hardwired to 0."""
        if reg == "R0":
            return 0
        return self.reg_file.regs.get(reg, 0)

    def run(self):
        """Main simulation loop."""
        max_cycles = 100000  # Safety limit

        while self.cycle < max_cycles:
            self.cycle += 1

            # Order of operations each cycle:
            # 1. Write Result (CDB broadcast) - must happen before issue
            #    so newly freed RS tags can be captured
            # 2. Execute (advance execution counters)
            # 3. Issue (send instruction from decode buffer to RS)
            # 4. Fetch/Decode (fetch new instruction into decode buffer)

            self._write_result()
            self._execute()
            self._issue()
            self._fetch_decode()

            # Check if simulation is done:
            # All instructions fetched, decode buffer empty, all RS idle
            if (self.all_fetched and
                    len(self.decode_buffer) == 0 and
                    all(not rs.busy for rs in self.rs_list)):
                break

        self.total_cycles = self.cycle
        return self.cycle

    def _fetch_decode(self):
        """
        Fetch up to NF instructions per cycle into decode buffer,
        unless stalled by branch or decode buffer full.
        """
        if self.all_fetched:
            return

        # If branch is pending, stall fetch
        if self.fetch_stalled_for_branch:
            return

        # If decode buffer is full, stall fetch
        if len(self.decode_buffer) >= self.NI:
            return

        # Fetch up to NF instructions
        fetched = 0
        while fetched < NF and len(self.decode_buffer) < self.NI:
            if self.pc >= len(self.instructions):
                self.all_fetched = True
                break
            instr = self.instructions[self.pc]
            self.decode_buffer.append({"instr": instr, "fetched_cycle": self.cycle})
            self.pc += 1
            fetched += 1

            # If it's a branch, stall fetch after this instruction
            if instr["op"] == "bne":
                self.fetch_stalled_for_branch = True
                break

    def _issue(self):
        """
        Try to issue up to NW instructions from decode buffer to free RS.
        """
        issued_this_cycle = 0

        while issued_this_cycle < NW and len(self.decode_buffer) > 0:
            entry = self.decode_buffer[0]
            instr = entry["instr"]
            op = instr["op"]

            fu_name = self.op_to_fu.get(op)
            if fu_name is None:
                print(f"[ERROR] No FU for op: {op}")
                self.decode_buffer.pop(0)
                continue

            # Find a free RS for this FU
            free_rs = None
            for rs in self.fu_rs[fu_name]:
                if not rs.busy:
                    free_rs = rs
                    break

            if free_rs is None:
                # Structural hazard - RS full, stall
                self.structural_stalls += 1
                break  # Stop issuing

            # Issue the instruction to free_rs
            self.decode_buffer.pop(0)
            self._fill_rs(free_rs, instr)
            issued_this_cycle += 1

    def _fill_rs(self, rs, instr):
        """Fill a reservation station with the given instruction."""
        op = instr["op"]
        rs.busy = True
        rs.op = op
        rs.done = False
        rs.result = None
        rs.exec_start = None
        rs.exec_remaining = None
        rs.issue_cycle = self.cycle
        rs.instr_index = self.issued_count
        self.issued_count += 1

        def get_operand(reg):
            """Return (value, tag) for a register operand."""
            if reg is None:
                return (0, None)
            tag = self.rat.get(reg)
            if tag is None:
                return (self.get_reg_value(reg), None)
            else:
                return (None, tag)

        if op == "addi":
            v, q = get_operand(instr["rs1"])
            rs.vj = v
            rs.qj = q
            rs.vk = instr["imm"]
            rs.qk = None
            rs.dest_reg = instr["rd"]
            self.rat.set(instr["rd"], rs.rs_id)

        elif op == "add":
            v, q = get_operand(instr["rs1"])
            rs.vj = v; rs.qj = q
            v, q = get_operand(instr["rs2"])
            rs.vk = v; rs.qk = q
            rs.dest_reg = instr["rd"]
            self.rat.set(instr["rd"], rs.rs_id)

        elif op == "fld":
            # address = imm + reg value
            v, q = get_operand(instr["mem_reg"])
            rs.vj = v; rs.qj = q
            rs.imm = instr["imm"]
            rs.dest_reg = instr["rd"]
            self.rat.set(instr["rd"], rs.rs_id)

        elif op == "fsd":
            # Data source
            v, q = get_operand(instr["rs1"])
            rs.vj = v; rs.qj = q
            # Address register
            v, q = get_operand(instr["mem_reg"])
            rs.vk = v; rs.qk = q
            rs.imm = instr["imm"]
            rs.dest_reg = None  # No destination register

        elif op in ("fadd", "fmul", "fdiv"):
            v, q = get_operand(instr["rs1"])
            rs.vj = v; rs.qj = q
            v, q = get_operand(instr["rs2"])
            rs.vk = v; rs.qk = q
            rs.dest_reg = instr["rd"]
            self.rat.set(instr["rd"], rs.rs_id)

        elif op == "bne":
            v, q = get_operand(instr["rs1"])
            rs.vj = v; rs.qj = q
            v, q = get_operand(instr["rs2"])
            rs.vk = v; rs.qk = q
            rs.imm = instr.get("target_label")  # label name
            rs.dest_reg = None
            self.branch_rs = rs

    def _execute(self):
        """Advance execution for all busy RS that are ready."""
        for rs in self.rs_list:
            if not rs.busy or rs.done:
                continue

            if rs.ready():
                if rs.exec_start is None:
                    # Start execution
                    rs.exec_start = self.cycle
                    rs.exec_remaining = rs.latency

                rs.exec_remaining -= 1

                if rs.exec_remaining == 0:
                    # Compute result
                    rs.result = self._compute(rs)
                    rs.done = True

    def _compute(self, rs):
        """Compute the result for a completed RS."""
        op = rs.op
        vj = rs.vj if rs.vj is not None else 0
        vk = rs.vk if rs.vk is not None else 0

        if op == "addi":
            return int(vj) + int(rs.vk)
        elif op == "add":
            return int(vj) + int(vk)
        elif op == "fld":
            addr = int(vj) + int(rs.imm)
            rs.addr = addr
            return self.memory.read(addr)
        elif op == "fsd":
            addr = int(vk) + int(rs.imm)
            rs.addr = addr
            return vj  # The value to store
        elif op == "fadd":
            return float(vj) + float(vk)
        elif op == "fmul":
            return float(vj) * float(vk)
        elif op == "fdiv":
            return float(vj) / float(vk) if vk != 0 else 0.0
        elif op == "bne":
            # Evaluate condition: bne rs1, rs2, label
            taken = (int(vj) != int(vk))
            return taken
        return 0

    def _write_result(self):
        """
        CDB broadcast: select the oldest completed instruction (by issue order),
        broadcast result, update RAT, register file, and waiting RS.
        Only NB=1 instruction can broadcast per cycle.
        """
        # Find all done RS entries
        done_rs = [rs for rs in self.rs_list if rs.busy and rs.done]
        if not done_rs:
            return

        # Priority: oldest by instr_index
        done_rs.sort(key=lambda r: r.instr_index)
        # Only NB can broadcast
        broadcasting = done_rs[:NB]

        for rs in broadcasting:
            result = rs.result
            op = rs.op
            dest = rs.dest_reg

            # Write to memory for stores
            if op == "fsd":
                self.memory.write(rs.addr, result)

            # Write to register file and clear RAT
            if dest is not None:
                self.reg_file.regs[dest] = result
                self.rat.clear(dest, rs.rs_id)

            # Forward to waiting RS via CDB
            for other_rs in self.rs_list:
                if not other_rs.busy:
                    continue
                if other_rs.qj == rs.rs_id:
                    other_rs.vj = result
                    other_rs.qj = None
                if other_rs.qk == rs.rs_id:
                    other_rs.vk = result
                    other_rs.qk = None

            # Handle branch resolution
            if op == "bne":
                taken = result
                if taken:
                    target_label = rs.imm
                    new_pc = self.label_map.get(target_label)
                    if new_pc is None:
                        print(f"[ERROR] Label '{target_label}' not found!")
                        new_pc = self.pc
                    self.pc = new_pc
                    # Flush decode buffer (branch not taken path was not speculated anyway)
                    self.decode_buffer.clear()
                    self.all_fetched = False
                else:
                    # Not taken - continue from current PC
                    pass
                self.fetch_stalled_for_branch = False
                self.branch_rs = None

            # Free the RS
            rs.reset()


def print_results(sim, instructions):
    print("\n" + "="*60)
    print("SIMULATION RESULTS")
    print("="*60)
    print(f"Total Execution Cycles : {sim.total_cycles}")
    print(f"Structural Stalls      : {sim.structural_stalls}")
    print(f"Instructions Executed  : {sim.issued_count}")

    print("\n--- Final Register File State (non-zero) ---")
    for reg, val in sorted(sim.reg_file.regs.items()):
        if val != 0:
            print(f"  {reg} = {val}")

    print("\n--- Final Memory State (non-zero) ---")
    for addr in sorted(sim.memory.mem.keys()):
        val = sim.memory.mem[addr]
        if val != 0:
            print(f"  mem[{addr}] = {val}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python tomasulo_simulator.py <input_file> [NI] [RS_SIZE]")
        print("  NI      = Decode buffer size (default: 1)")
        print("  RS_SIZE = Override RS count for all FUs (default: per-FU defaults)")
        sys.exit(1)

    filename = sys.argv[1]
    ni = int(sys.argv[2]) if len(sys.argv) > 2 else NI
    rs_size = int(sys.argv[3]) if len(sys.argv) > 3 else None

    # Build FU config, optionally overriding RS counts
    fu_config = {}
    for fu_name, (lat, rs_count, ops) in FU_CONFIG.items():
        count = rs_size if rs_size is not None else rs_count
        fu_config[fu_name] = (lat, count, ops)

    print(f"Loading: {filename}")
    print(f"NI={ni}, RS_SIZE={'default' if rs_size is None else rs_size}")

    instructions, memory, label_map = parse_input_file(filename)

    print(f"Parsed {len(instructions)} instructions")
    print(f"Label map: {label_map}")
    print(f"Initial memory: {dict(sorted(memory.mem.items()))}")

    sim = TomasuloSimulator(instructions, memory, label_map,
                            NI=ni, fu_config=fu_config)
    sim.run()
    print_results(sim, instructions)


if __name__ == "__main__":
    main()