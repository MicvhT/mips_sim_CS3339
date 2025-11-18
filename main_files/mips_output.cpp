#include "mips_output.h"
#include "mips_pipeline.h"   // or wherever WordMemory is defined
#include <sstream>

OutputManager::OutputManager() : debugMode(false) {}

OutputManager::~OutputManager() = default;

void OutputManager::enableDebugMode(bool debug) {
    debugMode = debug;
    std::cout << (debug ? "Debug mode: ENABLED\n\n" : "Debug mode: DISABLED\n\n");
}

void OutputManager::printFinalRegisters(const std::array<int32_t, 32>& regs) const {
    printHeader("FINAL REGISTER FILE");
    for (int i = 0; i < 32; i += 4) {
        printRegisterRow(i, std::min(i + 3, 31), regs);
    }
    printSeparator();
}

void OutputManager::printRegisterRow(int start, int end,
                                    const std::array<int32_t, 32>& regs) const {
    std::cout << std::left << std::setw(8) << "Reg"
              << std::setw(8) << "Name"
              << std::setw(12) << "Decimal"
              << std::setw(12) << "Hex" << std::endl;
    for (int i = start; i <= end; ++i) {
        int32_t val = regs[i];
        std::cout << std::left << std::setw(8) << ("$" + std::to_string(i))
                  << std::setw(8) << FULL_REG_NAMES[i]
                  << std::setw(12) << val
                  << "0x" << std::hex << std::setw(8) << std::setfill('0')
                  << (uint32_t)val << std::dec << std::setfill(' ') << std::endl;
    }
    std::cout << std::endl;
}

void OutputManager::printFinalMemory(const WordMemory& mem) const {
    printHeader("FINAL MEMORY CONTENTS");
    std::cout << "First 256 bytes (64 words) from 0x00000000:\n";
    printMemoryBlock(0, 64, mem);
}

void OutputManager::printMemoryBlock(uint32_t startAddr, int words,
                                    const WordMemory& mem) const {
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < words; i += 4) {
        uint32_t addr = startAddr + i * 4;
        std::cout << "0x" << std::setw(8) << addr << ": ";
        for (int j = 0; j < 4 && (i + j) < words; ++j) {
            uint32_t word = mem.load(addr + j * 4);  // assuming WordMemory has load()
            std::cout << std::setw(8) << word << " ";
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
    std::cout << "PC: 0x" << std::hex << pc << std::dec << "   Instruction: " << instruction << "\n\n";

    std::cout << "Registers $0-$15:\n";
    printRegisterRow(0, 15, regs);
    std::cout << std::endl;
}

void OutputManager::printHeader(const std::string& title) const {
    std::cout << "\n" << std::string(60, '=') << "\n " << title << "\n" << std::string(60, '=') << "\n";
}

void OutputManager::printSeparator() const {
    std::cout << std::string(80, '-') << std::endl;
}
