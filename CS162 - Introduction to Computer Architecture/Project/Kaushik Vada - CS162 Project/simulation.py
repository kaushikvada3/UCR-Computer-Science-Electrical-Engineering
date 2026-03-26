#!/usr/bin/env python3
# CS 162 tomasulo sim
# referenced the textbook ch3 a lot for this

import sys, json, math, re, os

FU_ORDER = ["INT","LS","FPADD","FPMUL","FPDIV","BU"]
FU_LATENCIES = {"INT":1,"LS":2,"FPADD":3,"FPMUL":4,"FPDIV":6,"BU":1}
RS_SIZES = {"INT":4,"LS":3,"FPADD":3,"FPMUL":2,"FPDIV":1,"BU":1}
FU_PREFIX = {"INT":"INT","LS":"LS","FPADD":"A","FPMUL":"M","FPDIV":"D","BU":"B"}
OP_TO_FU = {
    "add":"INT","addi":"INT","fld":"LS","fsd":"LS",
    "fadd":"FPADD","fmul":"FPMUL","fdiv":"FPDIV","bne":"BU"
}


class Instruction:
    def __init__(self, pc, op, text, srcLine):
        self.pc = pc
        self.op = op
        self.text = text
        self.srcLine = srcLine
        self.dest = None
        self.src1 = None; self.src2 = None
        self.base = None
        self.imm = 0
        self.target_label = None
        self.target_pc    = None


class RSEntry:
    def __init__(self, tag, fu_type):
        self.tag     = tag
        self.fu_type = fu_type
        self.busy    = False
        self.op      = None
        self.instr   = None
        self.issue_order = None
        self.issue_cycle = None
        self.destReg = None
        self.vj = None; self.vk = None
        self.qj = None; self.qk = None
        self.vj_ready_cycle = -1
        self.vk_ready_cycle = -1
        self.imm       = 0
        self.executing = False
        self.exec_rem  = 0
        self.completed       = False
        self.completed_cycle = None
        self.waitCDB = False
        self.result   = None
        self.eff_addr = None
        self.mem_done = False

    def clear(self):
        # lazy but works
        self.__init__(self.tag, self.fu_type)


class FunctionalUnit:
    def __init__(self, fu_type, latency):
        self.fu_type  = fu_type
        self.latency  = latency
        self.curr_tag = None
        self.rem      = 0

    def is_busy(self): return self.curr_tag != None

    def clear(self): self.curr_tag = None; self.rem = 0


def strip_comment(line):
    for ch in ('#', '//'):
        if ch in line: line = line[:line.find(ch)]
    return line.strip()


def to_int32(v):
    # wrap signed 32 bit
    v &= 0xFFFFFFFF
    if v & 0x80000000: v -= 0x100000000
    return v


def parse_reg(tok, kind, ln):
    tok = tok.strip().upper()
    if tok == "$0": tok = "R0"
    if not re.match(r"^[RF]\d+$", tok):
        print("bad register %s line %d" % (tok, ln)); sys.exit(1)
    pre = tok[0]; idx = int(tok[1:])
    if pre != kind or idx < 0 or idx > 31:
        print("bad register %s line %d" % (tok, ln)); sys.exit(1)
    return tok


def parse_file(path):
    try:
        lines = open(path).readlines()
    except:
        print("cant open:", path); sys.exit(1)

    program = []; memory = {}; labels = {}; ilines = []

    for idx, raw in enumerate(lines):
        ln   = idx + 1
        line = strip_comment(raw)
        if not line: continue

        m = re.match(r"^\s*([+-]?\d+)\s*,\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+))\s*$", line)
        if m:
            memory[int(m.group(1))] = float(m.group(2)); continue

        # handle labels
        rest = line
        while True:
            lm = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*:\s*(.*)$", rest)
            if not lm: break
            labels[lm.group(1)] = len(ilines)
            rest = lm.group(2).strip()
            if not rest: break
        if rest: ilines.append((ln, rest))

    # second pass now that labels are known
    for pc, (ln, text) in enumerate(ilines):
        pts = text.split(None, 1)
        op  = pts[0].lower()
        ops = [x.strip() for x in pts[1].split(",")] if len(pts) > 1 else []

        inst = Instruction(pc, op, text, ln)
        if op == "add":
            inst.dest = parse_reg(ops[0],"R",ln)
            inst.src1 = parse_reg(ops[1],"R",ln); inst.src2 = parse_reg(ops[2],"R",ln)
        elif op == "addi":
            inst.dest = parse_reg(ops[0],"R",ln)
            inst.src1 = parse_reg(ops[1],"R",ln); inst.imm = int(ops[2], 0)
        elif op in ["fadd","fmul","fdiv"]:
            inst.dest = parse_reg(ops[0],"F",ln)
            inst.src1 = parse_reg(ops[1],"F",ln); inst.src2 = parse_reg(ops[2],"F",ln)
        elif op == "fld":
            inst.dest = parse_reg(ops[0],"F",ln)
            mm = re.match(r"^\s*([+-]?\d+)\s*\(\s*([^)]+?)\s*\)\s*$", ops[1])
            inst.imm = int(mm.group(1)); inst.base = parse_reg(mm.group(2),"R",ln)
        elif op == "fsd":
            inst.src1 = parse_reg(ops[0],"F",ln)
            mm = re.match(r"^\s*([+-]?\d+)\s*\(\s*([^)]+?)\s*\)\s*$", ops[1])
            inst.imm = int(mm.group(1)); inst.base = parse_reg(mm.group(2),"R",ln)
        elif op == "bne":
            inst.src1 = parse_reg(ops[0],"R",ln); inst.src2 = parse_reg(ops[1],"R",ln)
            inst.target_label = ops[2]; inst.target_pc = labels[inst.target_label]
        else:
            print("unknown op:", op); sys.exit(1)
        program.append(inst)

    return program, memory, labels


class Simulator:
    def __init__(self, program, memory, labels, ni, trace, rs_sizes=None):
        self.program = program
        self.memory  = memory
        self.labels  = labels
        self.ni      = ni
        self.trace   = trace
        self.rs_sizes = rs_sizes if rs_sizes != None else RS_SIZES.copy()

        self.pc      = 0
        self.dq      = []      # decode queue
        self.stalled = False   # branch stall flag
        self.issue_ctr    = 0
        self.cycle        = 0
        self.total_cycles = 0
        self.stall_count  = 0

        # reg files
        self.ireg      = {"R"+str(i):0    for i in range(32)}
        self.freg      = {"F"+str(i):0.0  for i in range(32)}
        self.ireg_stat = {"R"+str(i):None for i in range(32)}
        self.freg_stat = {"F"+str(i):None for i in range(32)}

        # build RS banks
        self.rs = {}
        for fu in FU_ORDER:
            self.rs[fu] = []
            for i in range(self.rs_sizes[fu]):
                self.rs[fu].append(RSEntry("%s%d" % (FU_PREFIX[fu], i+1), fu))

        self.fus = {fu: FunctionalUnit(fu, FU_LATENCIES[fu]) for fu in FU_ORDER}

    def tr(self, msg):
        if self.trace: print(msg)

    def all_rs(self):
        res = []
        for fu in FU_ORDER: res += self.rs[fu]
        return res

    def done(self):
        if self.pc < len(self.program): return False
        if len(self.dq) > 0: return False
        if self.stalled: return False
        if any(u.is_busy() for u in self.fus.values()): return False
        if any(e.busy for e in self.all_rs()): return False
        return True

    # cycle order matters!! wb -> issue -> fetch -> ex_start -> ex_adv
    def run(self):
        while not self.done():
            self.cycle += 1
            if self.trace: print("Cycle %d" % self.cycle)
            self.do_wb()
            self.do_issue()
            self.do_fetch()
            self.do_ex_start()
            self.do_ex_adv()
        self.total_cycles = self.cycle

    def do_wb(self):
        readyList = [e for e in self.all_rs()
                     if e.waitCDB and e.completed_cycle != None and e.completed_cycle < self.cycle]
        if len(readyList) == 0:
            self.tr("  WB: nothing"); return

        # oldest gets the bus
        winner = min(readyList, key=lambda e: e.issue_order)

        vs = str(int(winner.result)) if winner.result.is_integer() else ("%.6f"%winner.result).rstrip("0").rstrip(".")
        self.tr("  WB: %s -> %s" % (winner.tag, vs))

        dest = winner.destReg
        if dest and dest != "R0":
            if dest[0] == "R":
                if self.ireg_stat[dest] == winner.tag:
                    self.ireg[dest] = to_int32(int(winner.result)); self.ireg_stat[dest] = None
            else:
                if self.freg_stat[dest] == winner.tag:
                    self.freg[dest] = float(winner.result); self.freg_stat[dest] = None

        # snoop CDB - wake up waiting RS entries
        for e in self.all_rs():
            if not e.busy: continue
            if e.qj == winner.tag:
                e.qj = None; e.vj = winner.result; e.vj_ready_cycle = self.cycle
            if e.qk == winner.tag:
                e.qk = None; e.vk = winner.result; e.vk_ready_cycle = self.cycle

        winner.clear()

    def do_issue(self):
        if len(self.dq) == 0:
            self.tr("  ISSUE: empty"); return

        inst = self.dq[0]
        fu   = OP_TO_FU[inst.op]
        slot = next((e for e in self.rs[fu] if not e.busy), None)
        if slot == None:
            self.stall_count += 1
            self.tr("  ISSUE: stall %s (%s full)" % (inst.text, fu)); return

        self.dq.pop(0); self.issue_ctr += 1
        slot.clear()
        slot.busy = True; slot.op = inst.op; slot.instr = inst
        slot.issue_order = self.issue_ctr; slot.issue_cycle = self.cycle
        slot.imm = inst.imm

        if inst.op == "add":
            self._setop(slot,"j",inst.src1,False); self._setop(slot,"k",inst.src2,False)
            slot.destReg = inst.dest
        elif inst.op == "addi":
            self._setop(slot,"j",inst.src1,False); slot.destReg = inst.dest
        elif inst.op in ["fadd","fmul","fdiv"]:
            self._setop(slot,"j",inst.src1,True); self._setop(slot,"k",inst.src2,True)
            slot.destReg = inst.dest
        elif inst.op == "fld":
            self._setop(slot,"j",inst.base,False); slot.destReg = inst.dest
        elif inst.op == "fsd":
            self._setop(slot,"j",inst.base,False); self._setop(slot,"k",inst.src1,True)
        elif inst.op == "bne":
            self._setop(slot,"j",inst.src1,False); self._setop(slot,"k",inst.src2,False)

        if slot.destReg and slot.destReg != "R0":
            if slot.destReg[0] == "R": self.ireg_stat[slot.destReg] = slot.tag
            else: self.freg_stat[slot.destReg] = slot.tag

        self.tr("  ISSUE: %s -> %s" % (inst.text, slot.tag))

    def _setop(self, slot, which, reg, fp):
        if reg == "R0":
            v = 0; t = None
        else:
            stat = self.freg_stat if fp else self.ireg_stat
            rf   = self.freg if fp else self.ireg
            t = stat[reg]; v = rf[reg] if t == None else None

        if which == "j":
            if t == None: slot.vj = float(v) if fp else int(v); slot.vj_ready_cycle = self.cycle
            else: slot.qj = t
        else:
            if t == None: slot.vk = float(v) if fp else int(v); slot.vk_ready_cycle = self.cycle
            else: slot.qk = t

    def do_fetch(self):
        if self.stalled: self.tr("  FETCH: branch stall"); return
        if len(self.dq) >= self.ni: self.tr("  FETCH: dq full"); return
        if self.pc >= len(self.program): self.tr("  FETCH: done"); return

        inst = self.program[self.pc]
        self.dq.append(inst)
        self.tr("  FETCH: [%d] %s" % (inst.pc, inst.text))
        self.pc += 1
        if inst.op == "bne": self.stalled = True  # stall until resolved

    def _ready(self, e):
        if not e.busy or e.executing or e.waitCDB: return False
        if e.issue_cycle >= self.cycle: return False  # need at least 1 cycle gap
        if e.op in ["add","fadd","fmul","fdiv","fsd","bne"]:
            return (e.qj == None and e.vj_ready_cycle < self.cycle) and \
                   (e.qk == None and e.vk_ready_cycle < self.cycle)
        if e.op in ["addi","fld"]:
            return e.qj == None and e.vj_ready_cycle < self.cycle
        return False

    def _mem_conflict(self, e):
        # older mem op still in flight? block
        for other in self.rs["LS"]:
            if not other.busy or other.tag == e.tag: continue
            if other.issue_order < e.issue_order and not other.mem_done: return True
        return False

    def do_ex_start(self):
        for fuName in FU_ORDER:
            unit = self.fus[fuName]
            if unit.is_busy(): continue
            cands = [e for e in self.rs[fuName] if self._ready(e)
                     and not (fuName == "LS" and self._mem_conflict(e))]
            if len(cands) == 0: self.tr("  EX-START %s: idle" % fuName); continue
            best = min(cands, key=lambda e: e.issue_order)
            best.executing = True; best.exec_rem = unit.latency
            unit.curr_tag = best.tag; unit.rem = unit.latency
            self.tr("  EX-START %s: %s (%s)" % (fuName, best.tag, best.instr.text))

    def do_ex_adv(self):
        for fuName in FU_ORDER:
            unit = self.fus[fuName]
            if not unit.is_busy(): continue
            curr = next(e for e in self.all_rs() if e.tag == unit.curr_tag)
            curr.exec_rem -= 1; unit.rem -= 1
            if curr.exec_rem == 0: self._finish(curr); unit.clear()

    def _finish(self, e):
        e.executing = False; e.completed = True; e.completed_cycle = self.cycle

        if e.op == "add":
            e.result = float(to_int32(int(e.vj) + int(e.vk))); e.waitCDB = True
        elif e.op == "addi":
            e.result = float(to_int32(int(e.vj) + e.imm)); e.waitCDB = True
        elif e.op == "fadd":
            e.result = float(e.vj + e.vk); e.waitCDB = True
        elif e.op == "fmul":
            e.result = float(e.vj * e.vk); e.waitCDB = True
        elif e.op == "fdiv":
            if e.vk == 0: print("div by zero lol"); sys.exit(1)
            e.result = float(e.vj / e.vk); e.waitCDB = True
        elif e.op == "fld":
            addr = to_int32(int(e.vj) + e.imm)
            e.eff_addr = addr; e.result = float(self.memory.get(addr, 0.0))
            e.waitCDB = True; e.mem_done = True
        elif e.op == "fsd":
            addr = to_int32(int(e.vj) + e.imm)
            self.memory[addr] = float(e.vk); e.eff_addr = addr; e.mem_done = True
            vs = str(int(e.vk)) if float(e.vk).is_integer() else ("%.6f"%e.vk).rstrip("0").rstrip(".")
            self.tr("  EX-DONE: %s stored %s -> [%d]" % (e.tag, vs, addr))
            e.clear(); return
        elif e.op == "bne":
            taken = int(e.vj) != int(e.vk)
            self.pc = e.instr.target_pc if taken else e.instr.pc + 1
            self.stalled = False
            self.tr("  EX-DONE: %s branch -> pc=%d" % (e.tag, self.pc))
            e.clear(); return

        self.tr("  EX-DONE: %s done, waiting CDB" % e.tag)


def fmt(v):
    return str(int(v)) if float(v).is_integer() else ("%.6f"%v).rstrip("0").rstrip(".")


def main():
    args = sys.argv[1:]
    if len(args) == 0:
        print("usage: python simulation.py <file> [--trace] [--ni N] [--all-rs N] [--json-out f]")
        sys.exit(1)

    filepath = args[0]
    trace = False; ni = 1; json_out = None
    rs = RS_SIZES.copy()

    i = 1
    while i < len(args):
        a = args[i]
        if   a == "--trace":    trace = True
        elif a == "--ni":       i += 1; ni = int(args[i])
        elif a == "--all-rs":   i += 1; n = int(args[i]); rs = {k:n for k in FU_ORDER}
        elif a == "--int-rs":   i += 1; rs["INT"]   = int(args[i])
        elif a == "--ls-rs":    i += 1; rs["LS"]    = int(args[i])
        elif a == "--fpadd-rs": i += 1; rs["FPADD"] = int(args[i])
        elif a == "--fpmul-rs": i += 1; rs["FPMUL"] = int(args[i])
        elif a == "--fpdiv-rs": i += 1; rs["FPDIV"] = int(args[i])
        elif a == "--bu-rs":    i += 1; rs["BU"]    = int(args[i])
        elif a == "--json-out": i += 1; json_out = args[i]
        i += 1

    prog, mem, labels = parse_file(filepath)
    sim = Simulator(prog, mem, labels, ni, trace, rs)
    sim.run()

    print("Program: " + filepath)
    print("Configuration: NI=%d, INT=%d, LS=%d, FPADD=%d, FPMUL=%d, FPDIV=%d, BU=%d" %
          (ni, rs["INT"], rs["LS"], rs["FPADD"], rs["FPMUL"], rs["FPDIV"], rs["BU"]))
    print("")
    print("Statistics")
    print("  Total execution cycles: %d" % sim.total_cycles)
    print("  Issue stall events (full RS): %d" % sim.stall_count)
    print("")
    print("Integer Registers")
    for i in range(0, 32, 4):
        row = ""
        for j in range(4): row += "  R%d=%d" % (i+j, sim.ireg["R%d"%(i+j)])
        print(row)
    print("\nFloating Point Registers")
    for i in range(0, 32, 4):
        row = ""
        for j in range(4): row += "  F%d=%s" % (i+j, fmt(sim.freg["F%d"%(i+j)]))
        print(row)
    print("\nFinal Memory Contents")
    if len(sim.memory) > 0:
        for k in sorted(sim.memory.keys()): print("  [%d] = %s" % (k, fmt(sim.memory[k])))
    else:
        print("  <empty>")
    print("  Unspecified memory addresses are treated as 0.0.")

    if json_out != None:
        out = {
            "total_cycles": sim.total_cycles,
            "issue_stall_events": sim.stall_count,
            "configuration": {"ni":ni, "rs_sizes":rs, "latencies":FU_LATENCIES},
            "integer_registers": sim.ireg,
            "floating_registers": sim.freg,
            "memory": [{"address":k,"value":v} for k,v in sorted(sim.memory.items())]
        }
        json.dump(out, open(json_out,"w"), indent=2)


if __name__ == "__main__":
    main()
