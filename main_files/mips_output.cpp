#include "mips_output.h"
#include <sstream>
#include <iomanip>

// FULL REGISTER NAMES (32 total) - SINGLE SOURCE OF TRUTH
const std::vector<std::string> OutputManager::FULL_REG_NAMES = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

OutputManager::OutputManager() : debugMode(false) {}

OutputManager::~OutputManager() {}

void OutputManager::enableDebugMode(bool debug) {
    debugMode = debug;
    std::cout << (debug ? "Debug mode: ENABLED" : "Debug mode: DISABLED") << std::endl << std::endl;
}

void OutputManager::printFinalRegisters(const RegisterFile& regs) const {
    printHeader("FINAL REGISTER FILE");

    // Print registers in rows of 4 - SAFE with FULL_REG_NAMES
    for (int i = 0; i < 32; i += 4) {
        printRegisterRow(i, std::min(i + 3, 31), regs);
    }
    printSeparator();
}

void OutputManager::printRegisterRow(int start, int end, const RegisterFile& regs) const {
    std::cout << std::left << std::setw(8) << "Reg"
        << std::setw(8) << "Name"
        << std::setw(12) << "Decimal"
        << std::setw(12) << "Hex" << std::endl;

    for (int i = start; i <= end; ++i) {
        int32_t value = regs.getRegister(i);
        std::string name = (i < FULL_REG_NAMES.size()) ? FULL_REG_NAMES[i] : "???";

        std::cout << std::left << std::setw(8) << ("$" + std::to_string(i))
            << std::setw(8) << name
            << std::setw(12) << value
            << "0x" << std::hex << std::setw(8) << std::setfill('0')
            << static_cast<unsigned int>(value)
            << std::dec << std::setfill(' ') << std::endl;
    }
    std::cout << std::endl;
}

void OutputManager::printFinalMemory(const Memory& mem) const {
    printHeader("FINAL MEMORY CONTENTS");
    std::cout << "Memory (showing first 256 bytes, address 0x00000000 - 0x000000FF):\n";
    printMemoryBlock(0, 256, mem);
}

void OutputManager::printMemoryBlock(int startAddr, int blockSize, const Memory& mem) const {
    std::cout << std::hex << std::setfill('0');

    for (int i = startAddr; i < startAddr + blockSize; i += 16) {
        std::cout << "0x" << std::setw(8) << i << ": ";

        for (int j = 0; j < 4 && (i + j * 4) < startAddr + blockSize; ++j) {
            int32_t word = mem.readWord(i + j * 4);
            std::cout << std::setw(8) << static_cast<unsigned int>(word) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::setfill(' ') << std::endl;
}

void OutputManager::printFinalState(const RegisterFile& regs, const Memory& mem) const {
    printFinalRegisters(regs);
    printFinalMemory(mem);
}

void OutputManager::printInstructionDebug(
    const std::string& instruction,
    const PipelineState& state,
    const RegisterFile& regs,
    const Memory& mem,
    int cycle) const {

    if (!debugMode) return;

    printSeparator();
    printInstructionInfo(instruction, cycle);
    printPipelineState(state);
    printControlSignals(state.getControlSignals());

    std::cout << "\n=== CURRENT REGISTER SNAPSHOT ===\n";
    std::cout << "PC: 0x" << std::hex << state.getPC() << std::dec << std::endl;

    // SAFE: Always use FULL_REG_NAMES
    printRegisterRow(0, 7, regs);  // $0-$7
    printRegisterRow(8, 15, regs); // $8-$15

    std::cout << "\n=== MEMORY CHANGES (if any) ===\n";
    if (state.hadMemoryWrite()) {
        printMemoryBlock(state.getLastMemoryAccess() - 32, 64, mem);
    }
    std::cout << std::endl;
}

void OutputManager::printInstructionInfo(const std::string& instruction, int cycle) const {
    std::cout << "\n=== CYCLE " << cycle << " ===" << std::endl;
    std::cout << "Instruction: " << instruction << std::endl;
}

void OutputManager::printPipelineState(const PipelineState& state) const {
    std::cout << "\n=== PIPELINE STATE ===\n";
    std::cout << std::left << std::setw(12) << "Stage"
        << std::setw(20) << "Instruction"
        << std::setw(15) << "PC"
        << std::setw(10) << "Status" << std::endl;

    const char* stageNames[] = { "IF", "ID", "EX", "MEM", "WB" };
    auto stages = state.getPipelineStages();

    for (int i = 0; i < 5; ++i) {
        std::cout << std::left << std::setw(12) << stageNames[i]
            << std::setw(20) << "-"
            << std::setw(15) << "-"
            << std::setw(10) << "Idle" << std::endl;
    }
}

void OutputManager::printControlSignals(const std::map<std::string, bool>& signals) const {
    std::cout << "\n=== CONTROL SIGNALS ===\n";
    std::cout << std::left << std::setw(20) << "Signal" << "Value" << std::endl;

    const std::vector<std::pair<std::string, std::string>> signalDescriptions = {
        {"RegDst", "Register destination (1=RD, 0=RT)"},
        {"ALUSrc", "ALU source (1=imm, 0=reg)"},
        {"MemtoReg", "Memory to register (1=mem, 0=ALU)"},
        {"RegWrite", "Register write enable"},
        {"MemRead", "Memory read enable"},
        {"MemWrite", "Memory write enable"},
        {"Branch", "Branch enable"},
        {"ALUOp0", "ALU operation control 0"},
        {"ALUOp1", "ALU operation control 1"}
    };

    for (const auto& desc : signalDescriptions) {
        auto it = signals.find(desc.first);
        if (it != signals.end()) {
            std::cout << std::left << std::setw(20) << desc.first
                << (it->second ? "1" : "0")
                << "  // " << desc.second << std::endl;
        }
    }
}

void OutputManager::printHeader(const std::string& title) const {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void OutputManager::printSeparator() const {
    std::cout << std::string(80, '-') << std::endl;
}
