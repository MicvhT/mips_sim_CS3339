#ifndef MIPS_OUTPUT_H
#define MIPS_OUTPUT_H

#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <map>
#include "mips_core.h"

class OutputManager {
public:
    OutputManager();
    ~OutputManager();

    void printFinalRegisters(const RegisterFile& regs) const;
    void printFinalMemory(const Memory& mem) const;
    void printFinalState(const RegisterFile& regs, const Memory& mem) const;

    void enableDebugMode(bool debug = true);
    void printInstructionDebug(
        const std::string& instruction,
        const PipelineState& state,
        const RegisterFile& regs,
        const Memory& mem,
        int cycle
    ) const;

    void printControlSignals(const std::map<std::string, bool>& signals) const;
    void printPipelineState(const PipelineState& state) const;

private:
    bool debugMode;
    static const std::vector<std::string> FULL_REG_NAMES;

    void printHeader(const std::string& title) const;
    void printSeparator() const;
    void printRegisterRow(int start, int end, const RegisterFile& regs) const;
    void printMemoryBlock(int startAddr, int blockSize, const Memory& mem) const;
    void printInstructionInfo(const std::string& instruction, int cycle) const;
};

#endif