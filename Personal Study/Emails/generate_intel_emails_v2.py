import csv

# Handcrafted inputs - REWRITTEN for "3rd Year Undergrad" + Strict "No Sign-off" Style.
# STRICT RESUME MAPPING:
# - Custom L1/L2 Cache (Parametric, LRU, Write-back/Write-allocate)
# - RISC-V Pipeline (IF, ID, ALU, Mem, WB)
# - Synopsys VCS & Verdi for verification
# - Synopsys Design Compiler for synthesis

EMAILS = {
    # 1. Yi-Chin Yan (Senior CPU Logic)
    "yi-chin.yan@intel.com": {
        "subject": "3rd year EE student building RISC-V cores // Instruction Fetch ?s",
        "body": "Hi Yi-Chin,\n\nI've been analyzing Intel's work on frontend throughput, and it really resonated with my recent project work. I'm a 3rd year Electrical Engineering student where I didn't just learn architecture theory—I built a 5-stage RISC-V processor in Verilog from scratch.\n\nI specifically designed a custom parameterizable L1/L2 cache controller, implementing LRU replacement and write-back/write-allocate policies to optimize memory access. I've also run my designs through Synopsys VCS for verification and Design Compiler for synthesis. I'm eager to bring this hands-on RTL experience to an internship on your team for Summer 2026.\n\nMy resume is attached, and I'd love to prove what I can do."
    },
    # 2. Munish Sharma (Engineering Manager)
    "munish.sharma@intel.com": {
        "subject": "Aspiring CPU engineer for Summer '26 (Experience with Verilog/Synthesis)",
        "body": "Hi Munish,\n\nI'm a 3rd year Electrical Engineering student who is dead serious about a career in CPU design. I see you lead engineering teams at Intel, and I wanted to pitch myself directly.\n\nI'm not just efficient in Verilog; I've implemented a full RISC-V core and a custom L1/L2 cache hierarchy with MESI-like coherence concepts. I verify my RTL using Synopsys VCS and Verdi, and I understand the push-and-pull of timing closure from running Design Compiler. I want to bring that technical grit to your team as a Summer 2026 intern.\n\nMy resume is attached, and I'd appreciate a quick look."
    },
    # 3. Yury Levin (Senior Logic Design)
    "yury.levin@intel.com": {
        "subject": "Questions from a student building a 5-stage pipeline",
        "body": "Hi Yury,\n\nI see you're a Senior Logic Design Engineer. I'm currently solving hazard detection challenges in my own 5-stage RISC-V CPU, and it's given me a huge appreciation for the control logic robustness required at Intel.\n\nI'm a 3rd year EE undergrad with strong hands-on skills—I've coded parametric L1/L2 cache controllers and validated them using Synopsys VCS/Verdi. I don't just write code; I verify it. I'm looking for a Summer 2026 internship to contribute to your logic design efforts.\n\nResume attached, and thanks for your time."
    },
    # 4. Amit Verma (Arch/Logic)
    "amit.verma@intel.com": {
        "subject": "Bridging micro-arch specs to RTL (Student Resume)",
        "body": "Hi Amit,\n\nI noticed you bridge the gap between Architecture and Logic Design. That's exactly where I'm focusing my degree—taking high-level micro-architectural specs and turning them into efficient, synthesizeable RTL.\n\nI've implemented a 5-stage RISC-V pipeline and a highly configurable L1/L2 cache subsystem (LRU, Write-back) in Verilog. I validate everything with directed tests in Synopsys VCS. I'm ready to apply this rigorous design flow to an internship on your team for Summer 2026.\n\nI've attached my resume and look forward to hearing from you."
    },
    # 5. Vishal Dhami (Senior Logic)
    "vishal.dhami@intel.com": {
        "subject": "Optimizing ALU critical paths // Internship interest",
        "body": "Hi Vishal,\n\nI've been working on optimizing the critical path of my RISC-V ALU, and it's given me a lot of respect for the timing closure work you do at Intel.\n\nI'm a 3rd year Electrical Engineering student who loves the grind of logic optimization. I've designed custom L1/L2 caches and run synthesis using Design Compiler to check area/timing trade-offs. I want to apply these skills to a real project as an intern on your team next summer (2026).\n\nMy resume is attached, and thanks for considering me."
    },
    # 6. Angel Chao (Senior Logic)
    "angel.chao@intel.com": {
        "subject": "Verifying Load/Store logic (Student project)",
        "body": "Hi Angel,\n\nI'm recently deep in the verification of the Load/Store unit for my RISC-V processor, specifically handling unaligned accesses. It's detailed work, but I enjoy drilling down into the waveforms in Verdi.\n\nI'm a 3rd year EE undergrad with solid RTL coding skills (Verilog) and exposure to Design Compiler for synthesis. I've built parameterizable caches and verified them with Synopsys tools. I'm looking for a Summer 2026 internship where I can help your team with unit design or verification.\n\nI've attached my resume and appreciate your time."
    },
    # 7. Srikanth Balaji (Siemens EDA -> Intel)
    "srikanth.balaji@intel.com": {
        "subject": "EDA tool experience & Logic Design // Student reaching out",
        "body": "Hi Srikanth,\n\nI saw your background with Siemens EDA. That mix is awesome—I'm a 3rd year EE student who actually enjoys the verification environment side of things, using Synopsys VCS and Verdi to debug my RTL.\n\nI've built a RISC-V core and a custom L1/L2 cache controller (Write-back/Write-allocate) from scratch. I know how to interpret synthesis reports from Design Compiler. I'd love to intern on your team in Summer 2026 to put this tool knowledge to work.\n\nResume attached, and I hope we can connect."
    },
    # 8. Timothy Nguyen (Senior Logic)
    "timothy.nguyen@intel.com": {
        "subject": "Low-power logic design // 3rd year EE student",
        "body": "Hi Timothy,\n\nI've been reading up on fine-grained clock gating, and I'm experimenting with it in my RISC-V designs design to reduce dynamic power. I know that's a huge focus for your team.\n\nI'm a 3rd year EE student who has implemented complex logic like parametric L1/L2 caches in Verilog. I check my work with Synopsys VCS and synthesize with Design Compiler. I want to get industry experience implementing low-power logic during an internship in Summer 2026.\n\nI've attached my resume and look forward to your thoughts."
    },
    # 9. Alejandro Lenero (Senior Logic)
    "alejandro.lenero@intel.com": {
        "subject": "Datapath verification challenges (Student project)",
        "body": "Hi Alejandro,\n\nI've been verifying the arithmetic datapath for my RISC-V core and realized that finding corner-case bugs in Verdi is an art form. I want to learn that art from the best.\n\nI'm a 3rd year Electrical Engineering student with experience in Verilog, simulation (Synopsys VCS), and synthesis (Design Compiler). I've built a custom L1/L2 cache hierarchy from spec to RTL. I'm looking for a Summer 2026 internship to contribute to meaningful design work on your team.\n\nMy resume is attached, and thanks for reading."
    },
    # 10. Swapnil Aggarwal (Senior Logic)
    "swapnil.aggarwal@intel.com": {
        "subject": "Multi-core interconnects // Internship inquiry",
        "body": "Hi Swapnil,\n\nI'm diving into multi-core coherence protocols in my classes, and I've implemented a Write-back/Write-allocate L1/L2 cache controller in Verilog. Seeing how that scales to Intel's mesh fabrics is my next goal.\n\nI'm a 3rd year EE student with a strong grip on computer architecture fundamentals and standard industry tools like Synopsys VCS and Design Compiler. I'm looking for a Summer 2026 internship to help design or verify on-chip logic.\n\nResume attached, and I appreciate your consideration."
    },
    # 11. Jakob Saxtorph (Senior Logic)
    "jakob.saxtorph@intel.com": {
        "subject": "Implementing FPU logic (Student with Verilog exp)",
        "body": "Hi Jakob,\n\nI've been implementing a basic FPU and handling the logic for IEEE 754 rounding modes. It's a beast to verify in waveforms, but I enjoy the rigor.\n\nI'm a 3rd year EE undergrad who has built a full RISC-V pipeline and custom L1/L2 caches. I verify with Synopsys VCS and synthesize with Design Compiler. I'd love to intern on your team for Summer 2026 and work on these complex arithmetic units.\n\nI've attached my resume and hope to speak with you soon."
    },
    # 12. Michael Louis (Senior Logic)
    "michael.louis@intel.com": {
        "subject": "Frontend logic and IPC // 3rd year Student",
        "body": "Hi Michael,\n\nI'm interested in how you design decoupling buffers to keep IPC high. In my own RISC-V processor, I've seen how critical fetch/decode bandwidth is to overall performance.\n\nI'm a 3rd year Electrical Engineering student proficient in Verilog logic design. I've implemented parametric L1/L2 caches and verified them using Synopsys VCS. I'm eager to bring these skills to an internship on your team for Summer 2026.\n\nResume attached, and thanks for the opportunity."
    },
    # 13. Udi Sherel (PD Director)
    "udi.sherel@intel.com": {
        "subject": "Aspiring Physical Design engineer (Exp with Design Compiler)",
        "body": "Hi Udi,\n\nI see you lead CPU Physical Design at Intel. I'm a 3rd year EE student who is fascinated by the backend—taking my Verilog designs and seeing the area/power results in Design Compiler.\n\nI've built a RISC-V core and custom L1/L2 caches, so I understand the logic structure before it hits physical implementation. I'm looking for a Summer 2026 internship where I can learn world-class PPA closure methodologies under your leadership.\n\nMy resume is attached, and I appreciate your time."
    },
    # 14. Radha Rudrappasamy (Verif Mgr)
    "radhapriyanka.rudrappasamy@intel.com": {
        "subject": "Writing better testbenches // SystemVerilog student",
        "body": "Hi Radha,\n\nI've been writing directed tests for my RISC-V core using SystemVerilog and debugging in Verdi. I'm starting to see the limits of directed testing and the need for constrained random verification.\n\nI'm a 3rd year EE student who loves breaking my own code. I've built and verifying custom L1/L2 cache controllers (LRU, Write-back) and want to scale that experience up. I'm looking for a Summer 2026 internship to write robust testbenches for your team.\n\nResume attached, and I look forward to connecting."
    },
    # 15. Akhila Ponnam (GPU Logic)
    "akhila.ponnam@intel.com": {
        "subject": "GPU Logic Design // EE Student with Verilog skills",
        "body": "Hi Akhila,\n\nI'm interested in the throughput challenges of GPU shader cores. I've designed a 5-stage RISC-V CPU and custom L1/L2 caches, so I understand pipeline flow, but the parallelism of GPUs is where I want to go next.\n\nI'm a 3rd year EE undergrad with strong Verilog coding skills and experience with Synopsys VCS and Design Compiler. I'm looking for a Summer 2026 internship to help design logic for Intel's graphics IPs.\n\nI've attached my resume and thanks for reviewing it."
    },
    # 16. Ramya Krish (Verif Mgr)
    "ramya.krish@intel.com": {
        "subject": "Functional coverage & verification // Student inquiry",
        "body": "Hi Ramya,\n\nI realized recently that my processor design works in simulation, but without functional coverage, I don't know what I'm missing. That's why I'm focusing on better verification strategies.\n\nI'm a 3rd year EE student who wants to be a Verification Engineer. I've verified my own parametrizable L1/L2 cache controller using Synopsys VCS/Verdi. I'm looking for a Summer 2026 internship to help your team find bugs before silicon.\n\nResume attached, and I appreciate your consideration."
    },
    # 17. Frank Zappulla (SoC Logic - Cornell)
    "frank.zappulla@intel.com": {
        "subject": "Fellow engineer building detailed RTL (saw your diverse background)",
        "body": "Hi Frank,\n\nI saw your path through NVIDIA, IBM, and now Intel. That kind of perspective on SoC design is exactly what I want to gain as I start my career.\n\nI'm a 3rd year EE student effectively working as a logic designer in my coursework—building RISC-V cores, writing Verilog for custom L1/L2 caches (LRU/Write-back), and running synthesis in Design Compiler. I'm looking for a Summer 2026 internship where I can contribute to real SoC logic and learn from your experience.\n\nMy resume is attached, and thanks for your time."
    },
    # 18. Kshitij Raj (DV - Formal/Security)
    "kshitij.raj@intel.com": {
        "subject": "Formal Verification interest // 3rd year EE Student",
        "body": "Hi Kshitij,\n\nI read about your work in Formal Validation. I've been verifying my RISC-V processor using standard VCS simulation, but the idea of mathematically proving security properties is incredibly compelling to me.\n\nI'm a 3rd year EE student with strong fundamentals in digital logic and Verilog (including custom cache designs). I want to move beyond standard directed testing and work on formal methods during a Summer 2026 internship on your team.\n\nResume attached, and I hope to hear from you."
    },
    # 19. Sangam Gambhir (PD)
    "sangam.gambhir@intel.com": {
        "subject": "Physical Synthesis // Student with Design Compiler exp",
        "body": "Hi Sangam,\n\nI've been using Synopsys Design Compiler to synthesize my RISC-V RTL, but I know that physical synthesis—handling placement awareness—is where the real performance is won.\n\nI'm a 3rd year EE student who wants to specialize in Physical Design. I've built and validated complex logic (L1/L2 caches) and am ready to learn the backend flow properly. I'm looking for a Summer 2026 internship to contribute to your execution.\n\nI've attached my resume and appreciate a look."
    },
    # 20. John Rake (PD)
    "john.rake@intel.com": {
        "subject": "Floorplanning & Thermal challenges // Student interest",
        "body": "Hi John,\n\nI'm learning that a good floorplan makes or breaks a chip. I want to get good at that spatial puzzle.\n\nI'm a 3rd year EE student with experience in RTL (RISC-V/Caches) and synthesis using Design Compiler. I understand the logic I'm implementing, which I think makes me a better physical designer. I'm looking for a Summer 2026 internship to help with physical design tasks.\n\nResume attached, and thanks for your time."
    },
    # 21. Ashika Kumar (PD Lead)
    "ashika.kumar@intel.com": {
        "subject": "Tape-out goals // 3rd year EE Student",
        "body": "Hi Ashika,\n\nI saw you're a PD Lead. The intensity of closing timing for tape-out is something I want to experience firsthand. \n\nI'm a 3rd year Electrical Engineering student familiar with Verilog (RISC-V cores) and synthesis (Design Compiler). I know how to check reports and iterate on the design. I'm looking for a Summer 2026 internship on a Physical Design team to help you hit closure.\n\nMy resume is attached, and I appreciate your consideration."
    },
    # 22. Swetha Karusala (PD)
    "swetha.karusala@intel.com": {
        "subject": "IR Drop & Power Grids // Physical Design interest",
        "body": "Hi Swetha,\n\nI've been studying how IR drop can kill timing even on a \"working\" design. It's made me realize how critical the power grid network really is.\n\nI'm a 3rd year EE student looking for a Summer 2026 internship. I have strong fundamentals in digital circuits—I've built custom L1/L2 caches in Verilog and synthesized them with Design Compiler. I'd love to help analyze and fix physical effects on your team.\n\nResume attached, and thanks for reading."
    },
    # 23. Tuan Do (PD)
    "tuan.do@intel.com": {
        "subject": "Fixing Setup/Hold violations // Student inquiry",
        "body": "Hi Tuan,\n\nI've spent hours fixing setup and hold violations in my own synthesis runs (using Design Compiler), and I actually enjoy the puzzle of it.\n\nI'm a 3rd year EE student who has built a RISC-V core and custom caches from scratch. I'm looking for an internship for Summer 2026 where I can apply this attention to detail to help your team close timing.\n\nMy resume is attached, and I appreciate your time."
    },
    # 24. Pavithra Maridi (PD)
    "pavithra.maridi@intel.com": {
        "subject": "Signal Integrity issues // 3rd Year EE",
        "body": "Hi Pavithra,\n\nI'm interested in how you handle signal integrity and crosstalk at advanced nodes. It seems like the physics is fighting against the logic at that scale.\n\nI'm a 3rd year EE student with a solid background in electromagnetic fields and digital design (Verilog/Synthesis). I've implemented a full RISC-V pipeline and I'm looking for a Summer 2026 internship to work on these physical layer challenges.\n\nResume attached, and I hope to connect soon."
    },
    # 25. Alex Levenzon (PD)
    "alex.levenzon@intel.com": {
        "subject": "Hierarchical Place & Route // Student interest",
        "body": "Hi Alex,\n\nI'm curious about the specific methodologies you use for Hierarchical P&R on massive chips. Managing that complexity is the kind of challenge I want to take on.\n\nI'm a 3rd year EE student with experience in Verilog (RISC-V, L1/L2 caches) and synthesis (Design Compiler). I'm looking for a Summer 2026 internship to help with the backend implementation of your designs.\n\nMy resume is attached, and thanks for your consideration."
    }
}

def main():
    csv_path = r"C:\Users\kaush\Documents\UCR-Computer-Science-Electrical-Engineering\Personal Study\Emails\intel_emails.csv"
    
    with open(csv_path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['Name', 'Email', 'Subject', 'Body'])
        
        for email, content in EMAILS.items():
            # Derive name: "firstname.lastname@..."
            local = email.split('@')[0]
            parts = local.split('.')
            if len(parts) >= 2:
                name = f"{parts[0].capitalize()} {parts[1].capitalize()}"
            else:
                name = local.capitalize()
                
            writer.writerow([name, email, content['subject'], content['body']])
            
    print(f"Generated {len(EMAILS)} emails to {csv_path}")

if __name__ == "__main__":
    main()
