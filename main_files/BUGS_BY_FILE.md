# Bugs by File with Fixes - UPDATED

## ✅ FIXED BUGS

### main.cpp
- ✅ **Bug 1:** Signed/unsigned comparison warning - FIXED (line 208)
- ✅ **Bug 2:** Header printed multiple times - FIXED (moved outside loop)
- ✅ **Bug 3:** HALT instruction not parsed - FIXED (added to opcode mapping, line 44)
- ✅ **Duplicate code:** Removed duplicate `trim()` function - now uses `mips_core.h`

### mips_core.h
- ✅ **Conflict:** Removed duplicate `IF_ID`, `ID_EX`, `EX_MEM`, `MEM_WB` structs - FIXED
- ✅ **Conflict:** Removed duplicate `Instruction` struct - FIXED

---

## 📄 main.cpp - REMAINING BUGS

### Bug 1: Jump Address Format Issue (Line 134)
**Location:** Line 134
**Status:** ⚠️ MINOR - Design choice
**Issue:** Parser expects byte address, but MIPS J uses word addresses
**Note:** Current implementation expects byte address. This is a design choice.

**Fix (if you want word addresses):**
```cpp
case Op::J:
    // J-type: J address (word address)
    uint32_t word_addr;
    if (iss >> word_addr) {
        instr.addr = word_addr;  // Store as word address
    }
    break;
```

---

## 📄 mips_pipeline.cpp - REMAINING BUGS

### Bug 1: Missing Include (Line 1-11) ⚠️ IMPORTANT
**Location:** Top of file
**Status:** ❌ NOT FIXED
**Issue:** Missing `#include "mips_ir.hpp"` so `Instruction` and `Op` types are undefined when compiled standalone

**Fix:** Add at the top:
```cpp
#include "mips_ir.hpp"
#include <array>
#include <cstdint>
// ... rest of includes
```

---

### Bug 2: Constructor Initialization Order Warning (Line 88) ⚠️ WARNING
**Location:** Line 88
**Status:** ❌ NOT FIXED
**Issue:** Member initialization order doesn't match declaration order

**Current:**
```cpp
MIPSPipeline(const vector<Instruction>& program,
             size_t memory_words = (1u << 16),
             bool trace = false)
    : prog_(program), mem_(memory_words), trace_(trace) {
```

**Fix:** Check member declaration order (around line 351-365) and match it:
```cpp
// If members are declared as: RegFile regs_{}; WordMemory mem_; vector<Instruction> prog_;
// Then initialize in that order:
MIPSPipeline(const vector<Instruction>& program,
             size_t memory_words = (1u << 16),
             bool trace = false)
    : mem_(memory_words), prog_(program), trace_(trace) {
    regs_.fill(0);
}
```

---

### Bug 3: Load-Use Hazard Detection Bug (Line 221) 🔴 CRITICAL
**Location:** Line 221
**Status:** ❌ NOT FIXED
**Issue:** Uses `RegDst` to determine load destination, but LW always uses `rt`, not `rd`

**Current (WRONG):**
```cpp
uint8_t load_dest = id_ex_.c.RegDst ? id_ex_.rd : id_ex_.rt; // LW uses rt
```

**Fix:**
```cpp
// LW always uses rt as destination, regardless of RegDst
uint8_t load_dest = id_ex_.rt;
```

---

### Bug 4: Register $0 Read Protection Missing (Lines 279-280) 🔴 CRITICAL
**Location:** Lines 279-280 in `decode_in_id()`
**Status:** ❌ NOT FIXED
**Issue:** Reads directly from registers without checking if register is $0

**Current (WRONG):**
```cpp
int32_t rs_val = regs_[ins.rs];
int32_t rt_val = regs_[ins.rt];
```

**Fix:**
```cpp
int32_t rs_val = (ins.rs == 0) ? 0 : regs_[ins.rs];
int32_t rt_val = (ins.rt == 0) ? 0 : regs_[ins.rt];
```

---

### Bug 5: Sign Extension Missing for Immediate Values (Line 153) 🔴 CRITICAL
**Location:** Line 153
**Status:** ❌ NOT FIXED
**Issue:** Immediate values are not sign-extended when used in ALU

**Current:**
```cpp
int32_t aluB = id_ex_.c.ALUSrc ? id_ex_.imm : fwdB;
```

**Fix:** Sign-extend the immediate:
```cpp
int32_t aluB = id_ex_.c.ALUSrc ? sign_extend_16(id_ex_.imm) : fwdB;
```

**Also fix in branch target calculation (Line 180):**
```cpp
// Current:
branch_target = id_ex_.pc + 4 + ((int32_t)id_ex_.imm << 2);

// Fixed (sign-extend first):
branch_target = id_ex_.pc + 4 + (sign_extend_16(id_ex_.imm) << 2);
```

---

### Bug 6: Jump Instruction Address Calculation (Lines 184-185) 🔴 CRITICAL
**Location:** Lines 184-185
**Status:** ❌ NOT FIXED
**Issue:** Jump target calculation is incorrect - address is already shifted in ID stage

**Current:**
```cpp
if (id_ex_.c.Jump) {
    branch_taken  = true;
    branch_target = (id_ex_.pc & 0xF0000000u) |
                    (id_ex_.imm & 0x0FFFFFFFu);
}
```

**Problem:** In ID stage (line 206), the address is already shifted: `(int32_t)(if_id_.instr.addr << 2)`

**Fix Option 1:** Since address is already shifted, use it directly:
```cpp
if (id_ex_.c.Jump) {
    branch_taken  = true;
    // id_ex_.imm already contains (addr << 2) from ID stage
    branch_target = (id_ex_.pc & 0xF0000000u) | (id_ex_.imm & 0x0FFFFFFCu);
}
```

**Fix Option 2 (Better):** Don't shift in ID stage, shift here:
```cpp
// In ID stage (line 206), change:
new_id_ex.imm = (if_id_.instr.op == Op::J)
                 ? (int32_t)(if_id_.instr.addr)  // Don't shift here
                 : if_id_.instr.imm;

// In EX stage (line 184-185), shift here:
if (id_ex_.c.Jump) {
    branch_taken  = true;
    branch_target = (id_ex_.pc & 0xF0000000u) | ((id_ex_.imm << 2) & 0x0FFFFFFCu);
}
```

---

## 📄 mips_parser.cpp - REMAINING BUGS

### Bug 1: Syntax Error - Missing Quotes (Line 1)
**Location:** Line 1
**Status:** ❌ NOT FIXED
**Issue:** Missing quotes in include statement

**Current (WRONG):**
```cpp
#include mips_core.h
```

**Fix:**
```cpp
#include "mips_core.h"
```

---

### Bug 2: Missing Type Definitions
**Location:** Throughout file
**Status:** ❌ NOT FIXED
**Issue:** References undefined types: `DecodedInstruction`, `R_TYPE`, `I_TYPE`, `J_TYPE`

**Fix:** Add type definitions at the top:
```cpp
#include "mips_core.h"

enum InstructionType {
    R_TYPE,
    I_TYPE,
    J_TYPE
};

struct DecodedInstruction {
    uint8_t opcode;
    InstructionType type;
    uint8_t rs, rt, rd;
    uint8_t shamt;
    uint8_t funct;
    int16_t immediate;  // signed for sign extension
    uint32_t address;
};

DecodedInstruction decode(uint32_t instruction) {
    // ... rest of code
}
```

**Note:** This file is currently unused (dead code). Consider removing it or fully implementing it.

---

## 📄 mips_output.h / mips_output.cpp - REMAINING BUGS

### Bug 1: References Non-Existent Classes
**Location:** Throughout both files
**Status:** ❌ NOT FIXED
**Issue:** References `PipelineState` class that doesn't exist, and wrong `RegisterFile`/`Memory` classes

**Problems:**
- `PipelineState` class doesn't exist
- Uses `RegisterFile` from `mips_core.h` but pipeline uses `RegFile` (array)
- Uses `Memory` from `mips_core.h` but pipeline uses `WordMemory`
- Methods like `getRegister()`, `readWord()`, `hadMemoryWrite()`, `getPC()`, etc. don't exist

**Status:** These files are completely unused (dead code). 

**Options:**
1. **Delete them** (recommended if not needed)
2. **Rewrite them** to work with actual pipeline classes:
   - Replace `RegisterFile` with `RegFile` (array)
   - Replace `Memory` with `WordMemory`
   - Remove all `PipelineState` references
   - Update method calls to match actual API

---

## 📄 mips_core.h - REMAINING ISSUES

### Bug 1: Mostly Unused Code
**Location:** Entire file
**Status:** ⚠️ MINOR - Code cleanup
**Issue:** Defines types that are never used by the pipeline

**Unused:**
- `Opcode` enum (pipeline uses `Op` from `mips_ir.hpp`)
- `RegisterFile` class (pipeline uses `RegFile` array)
- `Memory` class (pipeline uses `WordMemory`)
- `ALU` class (ALU operations are inline in pipeline)

**Only used:** `trim()` function (now used by `main.cpp`)

**Status:** Dead code. Consider removing or keeping only what's needed.

---

## 📄 README.md - REMAINING ISSUES

### Bug 1: Empty File
**Status:** ❌ NOT FIXED
**Issue:** README is completely empty

**Fix:** Add documentation:
```markdown
# MIPS Pipeline Simulator

## Description
Instruction-by-instruction simulator for a pipelined MIPS processor.

## Building
```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o mips_sim
```

## Usage
```bash
./mips_sim [input_file.asm]
```

If no file is provided, reads from stdin.

## Supported Instructions
- R-type: ADD, SUB, MUL, AND, OR, SLL, SRL
- I-type: ADDI, LW, SW, BEQ
- J-type: J
- Control: HALT, NOP

## Output
Displays final register file and memory contents after program execution.
```

---

## Summary by Priority

### 🔴 CRITICAL (Must Fix - Will cause incorrect execution):
1. **mips_pipeline.cpp Bug 3:** Load-use hazard detection (line 221)
2. **mips_pipeline.cpp Bug 4:** Register $0 read protection (lines 279-280)
3. **mips_pipeline.cpp Bug 5:** Sign extension for immediates (line 153, 180)
4. **mips_pipeline.cpp Bug 6:** Jump address calculation (lines 184-185)

### 🟡 IMPORTANT (Should Fix):
5. **mips_pipeline.cpp Bug 1:** Missing include
6. **mips_pipeline.cpp Bug 2:** Constructor initialization order warning

### 🟢 MINOR (Nice to Have):
7. **main.cpp Bug 1:** Jump address format (design choice)
8. **mips_parser.cpp:** Fix or remove (dead code)
9. **mips_output.h/cpp:** Fix or remove (dead code)
10. **mips_core.h:** Clean up unused code
11. **README.md:** Add documentation

---

## Total Bug Count

- **Fixed:** 5 bugs
- **Critical (Unfixed):** 4 bugs
- **Important (Unfixed):** 2 bugs
- **Minor (Unfixed):** 5 issues
- **Total Remaining:** 11 bugs/issues
