// mips_pipeline.cpp
// 5-stage pipelined MIPS simulator with hazards & forwarding.
// Build standalone test:
// g++ -std=c++17 -O2 -Wall -Wextra mips_pipeline.cpp -o mips_sim

#include "mips_pipeline.h"
#include "mips_ir.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
#include <tuple>

using namespace std;

// ---------------- minimal helpers ----------------
static inline int32_t sign_extend_16(int32_t x) {
    return static_cast<int16_t>(x);
}

// WordMemory implementation
WordMemory::WordMemory(size_t words) : data_(words, 0) {}

size_t WordMemory::words() const {
    return data_.size();
}

int32_t WordMemory::load_word(uint32_t byte_addr) const {
    if (byte_addr % 4 != 0) throw runtime_error("Unaligned LW");
    size_t idx = byte_addr / 4;
    if (idx >= data_.size()) throw runtime_error("Out-of-bounds LW");
    return data_[idx];
}

void WordMemory::store_word(uint32_t byte_addr, int32_t value) {
    if (byte_addr % 4 != 0) throw runtime_error("Unaligned SW");
    size_t idx = byte_addr / 4;
    if (idx >= data_.size()) throw runtime_error("Out-of-bounds SW");
    data_[idx] = value;
}

const std::vector<int32_t>& WordMemory::raw() const {
    return data_;
}

std::vector<int32_t>& WordMemory::raw() {
    return data_;
}

// ---------------- the simulator ----------------
MIPSPipeline::MIPSPipeline(const vector<Instruction>& program,
             size_t memory_words,
             bool trace)
    // Bug 2: respect member declaration order (regs_, mem_, prog_, ...)
    : mem_(memory_words),
      prog_(program),
      trace_(trace) {
    regs_.fill(0);
}

void MIPSPipeline::run() {
    while (!isHalted()) step();
}

void MIPSPipeline::step() {
        if (halted_) return;
        cycles_++;

        // ===== WB =====
        if (mem_wb_.valid && !mem_wb_.c.isNOP) {
            if (mem_wb_.c.RegWrite && mem_wb_.dest != 0) {
                int32_t val = mem_wb_.c.MemToReg ? mem_wb_.mem_data : mem_wb_.alu_out;
                regs_[mem_wb_.dest] = val;
            }
        }
        if (mem_wb_.valid && !mem_wb_.c.isNOP && last_retired_halt_)
            halted_ = true;

        // ===== MEM =====
        MEM_WB new_mem_wb{};
        new_mem_wb.c       = ex_mem_.c;
        new_mem_wb.valid   = ex_mem_.valid;
        new_mem_wb.dest    = ex_mem_.dest;
        new_mem_wb.alu_out = ex_mem_.alu_out;

        if (ex_mem_.valid && !ex_mem_.c.isNOP) {
            if (ex_mem_.c.MemRead)
                new_mem_wb.mem_data = mem_.load_word((uint32_t)ex_mem_.alu_out);
            if (ex_mem_.c.MemWrite)
                mem_.store_word((uint32_t)ex_mem_.alu_out, ex_mem_.rt_val_forwarded);
        }

        bool     flush_if_id = false;
        uint32_t redirect_pc = pc_;
        if (ex_mem_.valid && ex_mem_.c.Branch && ex_mem_.branch_taken) {
            redirect_pc = ex_mem_.branch_target;
            flush_if_id = true;
        }
        if (ex_mem_.valid && ex_mem_.c.Jump) {
            redirect_pc = ex_mem_.branch_target;
            flush_if_id = true;
        }

        // ===== EX =====
        EX_MEM new_ex_mem{};
        new_ex_mem.c     = id_ex_.c;
        new_ex_mem.valid = id_ex_.valid;
        new_ex_mem.dest  = id_ex_.c.RegDst ? id_ex_.rd : id_ex_.rt;

        // forwarding
        int32_t fwdA = id_ex_.rs_val;
        int32_t fwdB = id_ex_.rt_val;

        if (ex_mem_.valid && ex_mem_.c.RegWrite && ex_mem_.dest != 0) {
            if (ex_mem_.dest == id_ex_.rs) fwdA = ex_mem_.alu_out;
            if (ex_mem_.dest == id_ex_.rt) fwdB = ex_mem_.alu_out;
        }
        if (mem_wb_.valid && mem_wb_.c.RegWrite && mem_wb_.dest != 0) {
            int32_t wb_val = mem_wb_.c.MemToReg ? mem_wb_.mem_data : mem_wb_.alu_out;
            if (mem_wb_.dest == id_ex_.rs) fwdA = wb_val;
            if (mem_wb_.dest == id_ex_.rt) fwdB = wb_val;
        }

        int32_t aluA = fwdA;
        // Bug 5: sign-extend immediates before ALU use
        int32_t imm_se = sign_extend_16(id_ex_.imm);
        int32_t aluB   = id_ex_.c.ALUSrc ? imm_se : fwdB;

        int32_t  alu_out       = 0;
        bool     branch_taken  = false;
        uint32_t branch_target = 0;

        if (id_ex_.valid && !id_ex_.c.isNOP) {
            switch (id_ex_.c.ALUOp) {
                case 0: alu_out = aluA + aluB; break;           // ADD / address
                case 1: alu_out = aluA - aluB; break;           // SUB / BEQ/BNE compare
                case 2: alu_out = aluA & aluB; break;
                case 3: alu_out = aluA | aluB; break;
                case 4: alu_out = (aluA < aluB) ? 1 : 0; break;
                case 5: alu_out = fwdA * fwdB; break;           // MUL
                case 6: // SLL: shift rt by shamt (imm)
                    alu_out = (int32_t)((uint32_t)fwdB << (id_ex_.imm & 31));
                    break;
                case 7: // SRL: shift rt by shamt (imm)
                    alu_out = (int32_t)((uint32_t)fwdB >> (id_ex_.imm & 31));
                    break;
                default: alu_out = 0; break;
            }

            if (id_ex_.c.Branch) {
                bool is_beq = (curr_id_op_ == Op::BEQ);
                bool is_bne = (curr_id_op_ == Op::BNE);
                bool eq     = (fwdA == fwdB);
                branch_taken  = (is_beq && eq) || (is_bne && !eq);
                // Bug 5: sign-extend imm before shifting
                branch_target = id_ex_.pc + 4 +
                                (sign_extend_16(id_ex_.imm) << 2);
            }
            if (id_ex_.c.Jump) {
                branch_taken  = true;
                // Bug 6 (Option 2): imm holds the 26-bit word address; shift here
                uint32_t target = (id_ex_.imm & 0x03FFFFFFu) << 2;
                branch_target   = (id_ex_.pc & 0xF0000000u) | target;
            }
        }

        new_ex_mem.alu_out          = alu_out;
        new_ex_mem.rt_val_forwarded = fwdB;
        new_ex_mem.branch_taken     = branch_taken;
        new_ex_mem.branch_target    = branch_target;

        // ===== ID =====
        ID_EX new_id_ex{};
        if (if_id_.valid) {
            auto [ctrl, rs_val, rt_val] = decode_in_id(if_id_.instr);
            new_id_ex.c      = ctrl;
            new_id_ex.pc     = if_id_.pc;
            new_id_ex.rs     = if_id_.instr.rs;
            new_id_ex.rt     = if_id_.instr.rt;
            new_id_ex.rd     = if_id_.instr.rd;
            new_id_ex.rs_val = rs_val;
            new_id_ex.rt_val = rt_val;

            // Bug 6: don't shift J target here; keep raw 26-bit word index
            if (if_id_.instr.op == Op::J)
                new_id_ex.imm = static_cast<int32_t>(if_id_.instr.addr);
            else if (if_id_.instr.op == Op::SLL || if_id_.instr.op == Op::SRL)
                // SLL/SRL use shamt field, not imm
                new_id_ex.imm = static_cast<int32_t>(if_id_.instr.shamt);
            else
                new_id_ex.imm = if_id_.instr.imm;

            new_id_ex.valid = true;
            curr_id_op_     = if_id_.instr.op;
            last_retired_halt_ = (if_id_.instr.op == Op::HALT);
        } else {
            new_id_ex.c = MIPSPipeline::nop_ctrl();
            new_id_ex.valid = false;
            curr_id_op_ = Op::NOP;
            last_retired_halt_ = false;
        }

        // ===== hazard detection (load-use) =====
        bool stall = false;
        if (id_ex_.valid && id_ex_.c.MemRead) {
            // Bug 3: LW always writes RT, regardless of RegDst
            uint8_t load_dest = id_ex_.rt;
            if (load_dest != 0 &&
                (load_dest == if_id_.instr.rs ||
                 load_dest == if_id_.instr.rt)) {
                stall = true;
            }
        }

        // ===== IF =====
        IF_ID new_if_id{};
        uint32_t next_pc = pc_;
        if (flush_if_id) next_pc = redirect_pc;

        if (!stall) {
            if (next_pc / 4 < prog_.size()) {
                new_if_id.instr = prog_[next_pc / 4];
                new_if_id.pc    = next_pc;
                new_if_id.valid = true;
                next_pc += 4;
            } else {
                new_if_id.instr = Instruction{}; // NOP
                new_if_id.valid = false;
            }
        } else {
            // hold IF/ID, insert bubble into ID/EX
            new_if_id = if_id_;
            new_id_ex = {};
            new_id_ex.c     = MIPSPipeline::nop_ctrl();
            new_id_ex.valid = true;
        }

        if (flush_if_id) {
            new_if_id = {};
            new_if_id.instr = Instruction{};
            new_if_id.valid = false;
        }

        // commit all
        mem_wb_ = new_mem_wb;
        ex_mem_ = new_ex_mem;
        id_ex_  = new_id_ex;
        if_id_  = new_if_id;
        pc_     = next_pc;

        if (trace_) dump_trace_line();
}

bool MIPSPipeline::isHalted() const {
    return halted_;
}

uint64_t MIPSPipeline::cycles() const {
    return cycles_;
}

// ===== decode in ID =====
std::tuple<MIPSPipeline::Control, int32_t, int32_t> MIPSPipeline::decode_in_id(const Instruction& ins) {
        Control c{};
        c.isNOP = (ins.op == Op::NOP);

        // Bug 4: read protection for $0
        int32_t rs_val = (ins.rs == 0) ? 0 : regs_[ins.rs];
        int32_t rt_val = (ins.rt == 0) ? 0 : regs_[ins.rt];

        switch (ins.op) {
            case Op::ADD:
                c = {true,false,false,false,false,false,false,true,0,false};
                break;
            case Op::SUB:
                c = {true,false,false,false,false,false,false,true,1,false};
                break;
            case Op::AND:
                c = {true,false,false,false,false,false,false,true,2,false};
                break;
            case Op::OR:
                c = {true,false,false,false,false,false,false,true,3,false};
                break;
            case Op::SLT:
                c = {true,false,false,false,false,false,false,true,4,false};
                break;
            case Op::ADDI:
                c = {true,false,false,false,false,false,true,false,0,false};
                break;
            case Op::LW:   // and L if parser maps L -> Op::LW
                c = {true,true,false,true,false,false,true,false,0,false};
                break;
            case Op::SW:
                c = {false,false,true,false,false,false,true,false,0,false};
                break;
            case Op::BEQ:
                c = {false,false,false,false,true,false,false,false,1,false};
                break;
            case Op::BNE:
                c = {false,false,false,false,true,false,false,false,1,false};
                break;
            case Op::J:
                c = {false,false,false,false,false,true,false,false,0,false};
                break;
            case Op::HALT:
                c = {false,false,false,false,false,false,false,false,0,false};
                break;
            case Op::MUL:
                // rd = rs * rt
                c = {true,false,false,false,false,false,false,true,5,false};
                break;
            case Op::SLL:
                // rd = rt << shamt (imm)
                c = {true,false,false,false,false,false,true,true,6,false};
                break;
            case Op::SRL:
                // rd = rt >> shamt (imm)
                c = {true,false,false,false,false,false,true,true,7,false};
                break;
            case Op::NOP:
                c = MIPSPipeline::nop_ctrl();
                break;
        }
        return {c, rs_val, rt_val};
}

void MIPSPipeline::dump_trace_line() const {
    auto show = [&](const Instruction& i){ return i.str(); };

    cout << dec << "Cyc " << cycles_
        << " | PC=0x" << hex << pc_ << dec
        << " | IF: ";
    if (if_id_.valid) cout << show(if_id_.instr);
    else              cout << "-";

    cout << " | ID: "  << (id_ex_.valid  && !id_ex_.c.isNOP   ? "op" : "-")
        << " | EX: "  << (ex_mem_.valid && !ex_mem_.c.isNOP  ? "op" : "-")
        << " | MEM: " << (mem_wb_.valid && !mem_wb_.c.isNOP  ? "op" : "-")
        << "\n";
}

// Optional standalone test
#ifdef MIPS_PIPELINE_STANDALONE_MAIN
int main() {
    vector<Instruction> prog = {
        {Op::ADDI, 0, 8, 0, 4, 0},   // t0 = 4
        {Op::ADDI, 0, 9, 0, 3, 0},   // t1 = 3
        {Op::MUL,  8, 9,10, 0, 0},   // t2 = t0 * t1 = 12
        {Op::SLL,  0,10,11, 1, 0},   // t3 = t2 << 1 = 24
        {Op::SRL,  0,11,12, 2, 0},   // t4 = t3 >> 2 = 6
        {Op::HALT, 0, 0, 0, 0, 0}
    };

    MIPSPipeline sim(prog, 1024, true);
    sim.run();

    cout << "\nFinal registers:\n";
    const auto& R = sim.regs();
    for (int i = 0; i < 32; ++i)
        cout << "r" << setw(2) << setfill('0') << i << " = " << R[i] << "\n";

    return 0;
}
#endif
