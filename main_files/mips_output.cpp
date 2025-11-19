// mips_output.cpp
#include "mips_output.h"
#include "mips_pipeline.h"  

OutputManager::OutputManager() = default;
OutputManager::~OutputManager() = default;

void OutputManager::enableDebugMode(bool debug) {
    debugMode = debug;
    std::cout << (debug ? "Debug mode: ENABLED\n\n" : "Debug mode: DISABLED\n\n");
}

void OutputManager::printFinalRegisters(const std::array<int32_t, 32>& regs) const {
    printHeader("FINAL REGISTER FILE");

    // Header printed ONCE
    std::cout << std::left << std::setw(8) << "Reg"
              << std::setw(8) << "Name"
              << std::setw(12) << "Decimal"
              << std::setw(12) << "Hex" << std::endl;

    for (int i = 0; i < 32; i += 4) {
        printRegisterRow(i, std::min(i + 3, 31), regs);
    }
    printSeparator();
}

void OutputManager::printRegisterRow(int start, int end,
                                    const std::array<int32_t, 32>& regs) const {
    for (int i = start; i <= end; ++i) {
        int32_t val = regs[i];
        std::cout << std::left << std::setw(8) << ("$" + std::to_string(i))
                  << std::setw(8) << FULL_REG_NAMES[i]
                  << std::setw(12) << val
                  << "0x" << std::hex << std::setw(8) << std::setfill('0')
                  << static_cast<uint32_t>(val) << std::dec << std::setfill(' ')
                  << std::endl;
    }
    std::cout << std::endl;
}

void OutputManager::printFinalMemory(const WordMemory& mem) const {
    printHeader("FINAL MEMORY CONTENTS");
    std::cout << "Memory (showing first 256 bytes, address 0x00000000 - 0x000000FF):\n";
    printMemoryBlock(0, 256, mem);
}

void OutputManager::printMemoryBlock(uint32_t startAddr, int bytes,
                                    const WordMemory& mem) const {
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < bytes; i += 16) {
        uint32_t addr = startAddr + i;
        std::cout << "0x" << std::setw(8) << addr << ": ";

        for (int j = 0; j < 4 && (i + j * 4) < bytes; ++j) {
            int32_t word = mem.load_word(addr + j * 4);
            std::cout << std::setw(8) << static_cast<uint32_t>(word) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::setfill(' ') << std::endl;
}

void OutputManager::printFinalState(const std::array<int32_t, 32>& regs,
                                   const WordMemory& mem) const {
    printFinalRegisters(regs);
    std::cout << std::endl;
    printFinalMemory(mem);
}

void OutputManager::printInstructionDebug(const std::string& instruction,
                                          uint32_t pc,
                                          const std::array<int32_t, 32>& regs,
                                          int cycle) const {
    if (!debugMode) return;
    printSeparator();
    std::cout << "\n=== CYCLE " << cycle << " ===\n";
    std::cout << "PC: 0x" << std::hex << pc << std::dec
              << "   Instruction: " << instruction << "\n\n";
    printRegisterRow(0, 15, regs);
}

void OutputManager::printHeader(const std::string& title) const {
    std::cout << "\n" << std::string(60, '=') << "\n " << title << "\n"
              << std::string(60, '=') << "\n";
}

void OutputManager::printSeparator() const {
    std::cout << std::string(80, '-') << std::endl;
}
