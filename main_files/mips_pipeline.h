// mips_pipeline.h
#ifndef MIPS_PIPELINE_H
#define MIPS_PIPELINE_H

#include "mips_ir.hpp"
#include <array>
#include <cstdint>
#include <vector>

// Type alias for register file
using RegFile = std::array<int32_t, 32>;

// Word memory class - needed by mips_output.cpp
class WordMemory {
public:
    explicit WordMemory(size_t words);
    size_t words() const;
    int32_t load_word(uint32_t byte_addr) const;
    void store_word(uint32_t byte_addr, int32_t value);
    const std::vector<int32_t>& raw() const;
    std::vector<int32_t>& raw();

private:
    std::vector<int32_t> data_;
};

// Main pipeline class - needed by main.cpp
class MIPSPipeline {
public:
    MIPSPipeline(const std::vector<Instruction>& program,
                 size_t memory_words = (1u << 16),
                 bool trace = false);

    void run();
    void step();
    bool isHalted() const;

    // Public members (accessed directly by main.cpp)
    RegFile regs_;
    WordMemory mem_;
    
    // Public accessor
    uint64_t cycles() const;

private:
    std::vector<Instruction> prog_;
    uint32_t pc_{0};
    uint64_t cycles_{0};
    bool trace_{false};
    bool halted_{false};
    bool last_retired_halt_{false};
    Op curr_id_op_{Op::NOP};
    
    // Internal structures (full definitions needed for member access)
public:
    struct Control {
        bool RegWrite{false};
        bool MemRead{false};
        bool MemWrite{false};
        bool MemToReg{false};
        bool Branch{false};
        bool Jump{false};
        bool ALUSrc{false};
        bool RegDst{false};
        uint8_t ALUOp{0};
        bool isNOP{true};
    };
    
    static Control nop_ctrl() {
        Control c{};
        c.isNOP = true;
        return c;
    }

private:
    
    struct IF_ID {
        Instruction instr{};
        uint32_t pc{0};
        bool valid{false};
    };
    
    struct ID_EX {
        Control c{};
        uint32_t pc{0};
        int32_t rs_val{0}, rt_val{0};
        uint8_t rs{0}, rt{0}, rd{0};
        int32_t imm{0};
        bool valid{false};
        bool is_halt{false};  // Track if this instruction is a HALT
    };
    
    struct EX_MEM {
        Control c{};
        int32_t alu_out{0};
        int32_t rt_val_forwarded{0};
        uint8_t dest{0};
        bool branch_taken{false};
        uint32_t branch_target{0};
        bool valid{false};
        bool is_halt{false};  // Track if this instruction is a HALT
    };
    
    struct MEM_WB {
        Control c{};
        int32_t mem_data{0};
        int32_t alu_out{0};
        uint8_t dest{0};
        bool valid{false};
        bool is_halt{false};  // Track if this instruction is a HALT
    };
    
    IF_ID if_id_{};
    ID_EX id_ex_{};
    EX_MEM ex_mem_{};
    MEM_WB mem_wb_{};
    
    std::tuple<Control, int32_t, int32_t> decode_in_id(const Instruction& ins);
    void dump_trace_line() const;
};

#endif // MIPS_PIPELINE_H

