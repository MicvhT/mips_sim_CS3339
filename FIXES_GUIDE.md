# Fixes Guide: Making mips_output.cpp, mips_output.h, and main.cpp Work Together

This document explains all the bugs and how to fix them so that `mips_output.cpp`, `mips_output.h`, and `main.cpp` work together properly.

---

## Overview of Bugs

### mips_output.cpp
1. **Line 2**: Includes non-existent `"mips_pipeline.h"` 
2. **Line 3**: Unused include `<sstream>`
3. **Line 24-27**: Header printed multiple times (inside `printRegisterRow()`)
4. **Line 48-55**: Incorrect memory block printing logic
5. **Line 52**: Wrong method name - uses `mem.load()` instead of `mem.load_word()`

### mips_output.h
- No bugs found (this file is correct)

### main.cpp
1. **Line 151**: Unused variable `program`
2. **Line 197-213**: Manual memory printing (should use `OutputManager::printFinalMemory()` after fixes)

---

## Detailed Fixes

### Fix 1: mips_output.cpp - Line 2 (Include Statement)

**Problem:**
```cpp
#include "mips_pipeline.h"   // or wherever WordMemory is defined
```
This file doesn't exist. `WordMemory` is defined in `mips_pipeline.cpp`.

**Solution:**
Change to:
```cpp
#include "mips_pipeline.cpp"  // WordMemory is defined here
```

**Why:** Since `main.cpp` already includes `mips_pipeline.cpp` before `mips_output.cpp`, including it here ensures `WordMemory` is fully defined when `mips_output.cpp` tries to use it.

---

### Fix 2: mips_output.cpp - Line 3 (Unused Include)

**Problem:**
```cpp
#include <sstream>
```
This include is not used anywhere in the file.

**Solution:**
Remove this line entirely.

---

### Fix 3: mips_output.cpp - Lines 14-20 (Header Printed Multiple Times)

**Problem:**
The `printRegisterRow()` function prints the header ("Reg", "Name", "Decimal", "Hex") every time it's called. Since it's called 8 times (once per 4 registers), the header appears 8 times.

**Current Code:**
```cpp
void OutputManager::printFinalRegisters(const std::array<int32_t, 32>& regs) const {
    printHeader("FINAL REGISTER FILE");
    for (int i = 0; i < 32; i += 4) {
        printRegisterRow(i, std::min(i + 3, 31), regs);
    }
    printSeparator();
}

void OutputManager::printRegisterRow(int start, int end,
                                    const std::array<int32_t, 32>& regs) const {
    std::cout << std::left << std::setw(8) << "Reg"      // ❌ Header printed here
              << std::setw(8) << "Name"
              << std::setw(12) << "Decimal"
              << std::setw(12) << "Hex" << std::endl;
    // ... rest of function
}
```

**Solution:**
Move the header printing to `printFinalRegisters()` so it's printed only once:

```cpp
void OutputManager::printFinalRegisters(const std::array<int32_t, 32>& regs) const {
    printHeader("FINAL REGISTER FILE");
    // Print header once
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
    // Remove header printing from here - it's now in printFinalRegisters()
    for (int i = start; i <= end; ++i) {
        int32_t val = regs[i];
        std::cout << std::left << std::setw(8) << ("$" + std::to_string(i))
                  << std::setw(8) << FULL_REG_NAMES[i]
                  << std::setw(12) << std::dec << val
                  << "0x" << std::hex << std::setw(8) << std::setfill('0')
                  << static_cast<uint32_t>(val)
                  << std::dec << std::setfill(' ') << std::endl;
    }
    std::cout << std::endl;
}
```

---

### Fix 4: mips_output.cpp - Lines 45-58 (Memory Block Printing Logic)

**Problem:**
The loop logic is incorrect. The function iterates `i` by 4 but treats it as if incrementing by 1. Also, the condition `(i + j) < words` is wrong.

**Current Code:**
```cpp
void OutputManager::printMemoryBlock(uint32_t startAddr, int words,
                                    const WordMemory& mem) const {
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < words; i += 4) {  // ❌ i increments by 4
        uint32_t addr = startAddr + i * 4;  // ❌ But then multiplies by 4 again
        std::cout << "0x" << std::setw(8) << addr << ": ";
        for (int j = 0; j < 4 && (i + j) < words; ++j) {  // ❌ Wrong condition
            uint32_t word = mem.load_word(addr + j * 4);
            std::cout << std::setw(8) << word << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::setfill(' ') << std::endl;
}
```

**Solution:**
Fix the loop to correctly iterate through byte addresses:

```cpp
void OutputManager::printMemoryBlock(uint32_t startAddr, int words,
                                    const WordMemory& mem) const {
    std::cout << std::hex << std::setfill('0');
    // words is number of words, so we iterate by 16 bytes (4 words) at a time
    for (int i = 0; i < words * 4; i += 16) {  // Iterate by 16 bytes (4 words)
        uint32_t addr = startAddr + i;
        std::cout << "0x" << std::setw(8) << addr << ": ";
        for (int j = 0; j < 4 && (i + j * 4) < words * 4; ++j) {  // j is word index
            int32_t word = mem.load_word(addr + j * 4);
            std::cout << std::setw(8) << static_cast<uint32_t>(word) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec << std::setfill(' ') << std::endl;
}
```

**Alternative (simpler) solution** - match the format in main.cpp:
```cpp
void OutputManager::printMemoryBlock(uint32_t startAddr, int bytes,
                                    const WordMemory& mem) const {
    std::cout << std::hex << std::setfill('0');
    for (int i = 0; i < bytes; i += 16) {  // Iterate by 16 bytes
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
```

And update `printFinalMemory()`:
```cpp
void OutputManager::printFinalMemory(const WordMemory& mem) const {
    printHeader("FINAL MEMORY CONTENTS");
    std::cout << "Memory (showing first 256 bytes, address 0x00000000 - 0x000000FF):\n";
    printMemoryBlock(0, 256, mem);  // Pass 256 bytes, not 64 words
}
```

---

### Fix 5: mips_output.cpp - Line 52 (Wrong Method Name)

**Problem:**
```cpp
uint32_t word = mem.load(addr + j * 4);  // assuming WordMemory has load()
```
`WordMemory` has `load_word()`, not `load()`.

**Solution:**
Change to:
```cpp
int32_t word = mem.load_word(addr + j * 4);
```

Also change the type from `uint32_t` to `int32_t` to match the return type of `load_word()`.

---

### Fix 6: main.cpp - Line 151 (Unused Variable)

**Problem:**
```cpp
vector<Instruction> program;  // ❌ Never used
```

**Solution:**
Remove this line entirely.

---

### Fix 7: main.cpp - Lines 197-213 (Use OutputManager for Memory)

**Problem:**
After fixing `mips_output.cpp`, we can use `OutputManager::printFinalMemory()` instead of manual printing.

**Current Code:**
```cpp
// Print memory - work around mips_output.cpp bug (uses mem.load() instead of mem.load_word())
// So we'll print memory manually to match the original format
cout << "\n" << string(60, '=') << endl;
cout << " FINAL MEMORY CONTENTS" << endl;
cout << string(60, '=') << endl;
cout << "Memory (showing first 256 bytes, address 0x00000000 - 0x000000FF):\n";

cout << hex << setfill('0');
for (int i = 0; i < 256; i += 16) {
    cout << "0x" << setw(8) << i << ": ";
    for (int j = 0; j < 4 && (i + j * 4) < 256; ++j) {
        int32_t word = mem.load_word(i + j * 4);
        cout << setw(8) << static_cast<unsigned int>(word) << " ";
    }
    cout << endl;
}
cout << dec << setfill(' ') << endl;
```

**Solution:**
Replace with:
```cpp
// Print memory using OutputManager
output.printFinalMemory(mem);
```

---

## Complete Fixed Code

### mips_output.cpp (Fixed)

```cpp
#include "mips_output.h"
#include "mips_pipeline.cpp"  // WordMemory is defined here

OutputManager::OutputManager() : debugMode(false) {}

OutputManager::~OutputManager() = default;

void OutputManager::enableDebugMode(bool debug) {
    debugMode = debug;
    std::cout << (debug ? "Debug mode: ENABLED\n\n" : "Debug mode: DISABLED\n\n");
}

void OutputManager::printFinalRegisters(const std::array<int32_t, 32>& regs) const {
    printHeader("FINAL REGISTER FILE");
    // Print header once
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
                  << std::setw(12) << std::dec << val
                  << "0x" << std::hex << std::setw(8) << std::setfill('0')
                  << static_cast<uint32_t>(val)
                  << std::dec << std::setfill(' ') << std::endl;
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
```

### main.cpp (Fixed - relevant section)

```cpp
int main(int argc, char* argv[]) {
    // Read program from file or stdin
    istream* input = &cin;
    ifstream file;
    
    if (argc > 1) {
        file.open(argv[1]);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << argv[1] << endl;
            return 1;
        }
        input = &file;
    }
    
    // Parse instructions directly to IR format
    vector<Instruction> irProgram;
    string line;
    while (getline(*input, line)) {
        Instruction instr = parseInstruction(line);
        if (instr.op != Op::NOP) {
            irProgram.push_back(instr);
        }
    }
    
    if (file.is_open()) {
        file.close();
    }
    
    if (irProgram.empty()) {
        cerr << "Error: No valid instructions found" << endl;
        return 1;
    }
    
    // Create and run pipeline using MIPSPipeline from mips_pipeline.cpp
    MIPSPipeline pipeline(irProgram, 1024, false);
    pipeline.run();
    
    // Use OutputManager for output
    OutputManager output;
    const auto& regs = pipeline.regs();
    const auto& mem = pipeline.memory();
    
    // Print registers using OutputManager
    output.printFinalRegisters(regs);
    
    // Print memory using OutputManager
    output.printFinalMemory(mem);
    
    cout << "\nTotal cycles: " << pipeline.cycles() << endl;
    
    return 0;
}
```

---

## Summary of Changes

### mips_output.cpp
1. ✅ Changed `#include "mips_pipeline.h"` → `#include "mips_pipeline.cpp"`
2. ✅ Removed unused `#include <sstream>`
3. ✅ Moved header printing from `printRegisterRow()` to `printFinalRegisters()`
4. ✅ Fixed `printMemoryBlock()` loop logic
5. ✅ Changed `mem.load()` → `mem.load_word()`
6. ✅ Changed `uint32_t word` → `int32_t word`
7. ✅ Updated `printFinalMemory()` to pass bytes instead of words

### main.cpp
1. ✅ Removed unused variable `program`
2. ✅ Replaced manual memory printing with `output.printFinalMemory(mem)`

### mips_output.h
- ✅ No changes needed (file is correct)

---

## Testing

After applying these fixes:
1. The program should compile without errors
2. Register output should show header only once
3. Memory output should display correctly
4. All files should work together seamlessly

---

## Build Command

```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o mips_sim
```

Or if compiling separately:
```bash
g++ -std=c++17 -Wall -Wextra -c mips_output.cpp -o mips_output.o
g++ -std=c++17 -Wall -Wextra main.cpp mips_output.o -o mips_sim
```

