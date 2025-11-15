#ifndef MIPS_IR_HPP
#define MIPS_IR_HPP

#include <string>
#include <sstream>
#include <cstdint>

// Bridge between mips_core.h types and mips_pipeline.cpp expectations
// Note: mips_pipeline.cpp expects Instruction, Op from mips_ir.hpp
// We define them here to match what mips_pipeline.cpp needs

enum class Op {
    ADD, ADDI, SUB, MUL, AND, OR, SLL, SRL, SLT,
    LW, SW, BEQ, BNE, J, HALT, NOP
};

// Forward declare to avoid conflict - mips_pipeline.cpp will use this
struct IRInstruction {
    Op op = Op::NOP;
    uint8_t rs = 0, rt = 0, rd = 0;
    int32_t imm = 0;
    uint32_t addr = 0;
    
    std::string str() const {
        std::ostringstream oss;
        switch (op) {
            case Op::ADD:  oss << "ADD"; break;
            case Op::ADDI: oss << "ADDI"; break;
            case Op::SUB:  oss << "SUB"; break;
            case Op::MUL:  oss << "MUL"; break;
            case Op::AND:  oss << "AND"; break;
            case Op::OR:   oss << "OR"; break;
            case Op::SLL:  oss << "SLL"; break;
            case Op::SRL:  oss << "SRL"; break;
            case Op::SLT:  oss << "SLT"; break;
            case Op::LW:   oss << "LW"; break;
            case Op::SW:   oss << "SW"; break;
            case Op::BEQ:  oss << "BEQ"; break;
            case Op::BNE:  oss << "BNE"; break;
            case Op::J:    oss << "J"; break;
            case Op::HALT: oss << "HALT"; break;
            case Op::NOP:  oss << "NOP"; break;
        }
        return oss.str();
    }
};

// Alias for mips_pipeline.cpp compatibility  
using Instruction = IRInstruction;

#endif // MIPS_IR_HPP
