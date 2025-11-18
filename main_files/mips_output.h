#ifndef MIPS_OUTPUT_H
#define MIPS_OUTPUT_H

#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <iomanip>
#include <cstdint>

class WordMemory;  // Forward decl (defined in mips_pipeline.cpp or wherever)

class OutputManager {
public:
    OutputManager();
    ~OutputManager();

    void enableDebugMode(bool debug = true);

    // Current real types
    void printFinalRegisters(const std::array<int32_t, 32>& regs) const;
    void printFinalMemory(const WordMemory& mem) const;
    void printFinalState(const std::array<int32_t, 32>& regs,
                         const WordMemory& mem) const;

    // Simple debug per cycle (no PipelineState needed)
    void printInstructionDebug(
        const std::string& instruction,
        uint32_t pc,
        const std::array<int32_t, 32>& regs,
        int cycle) const;

private:
    bool debugMode;

    static constexpr const char* FULL_REG_NAMES[32] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };

    void printHeader(const std::string& title) const;
    void printSeparator() const;
    void printRegisterRow(int start, int end, const std::array<int32_t, 32>& regs) const;
    void printMemoryBlock(uint32_t startAddr, int words, const WordMemory& mem) const;
};

#endif
