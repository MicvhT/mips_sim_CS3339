#ifndef MIPS_CORE_H_INCLUDED
#define MIPS_CORE_H_INCLUDED

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <optional>
#include <algorithm>

using namespace std;

constexpr int NUM_REGS = 32;
constexpr int MEM_SIZE = 1024; // (4 bytes per word)


enum class Opcode {
    ADD, ADDI, SUB, MUL, AND, OR,
    SLL, SRL, LW, SW, BEQ, J, NOP, INVALID
};


class RegisterFile {
private:
    array<int32_t, NUM_REGS> regs{};

public:
    RegisterFile() { regs.fill(0); }

    int32_t read(int reg) const {
        if (reg == 0) return 0; // $zero is always 0
        return regs[reg];
    }

    void write(int reg, int32_t value) {
        if (reg != 0)
            regs[reg] = value;
    }

    void dump() const {
        cout << "=== Register File ===\n";
        for (int i = 0; i < NUM_REGS; ++i)
            cout << "R" << setw(2) << i << ": " << regs[i] << "\n";
    }
};

class Memory {
private:
    vector<int32_t> mem;

public:
    explicit Memory(size_t size = MEM_SIZE) : mem(size, 0) {}

    int32_t loadWord(uint32_t address) const {
        size_t idx = address / 4;
        if (idx >= mem.size()) return 0;
        return mem[idx];
    }

    void storeWord(uint32_t address, int32_t value) {
        size_t idx = address / 4;
        if (idx < mem.size())
            mem[idx] = value;
    }

    void dump() const {
        cout << "=== Memory (non-zero) ===\n";
        for (size_t i = 0; i < mem.size(); ++i)
            if (mem[i] != 0)
                cout << "[" << (i * 4) << "]: " << mem[i] << "\n";
    }
};

class ALU {
public:
    static int32_t operate(const string &op, int32_t a, int32_t b) {
        if (op == "ADD" || op == "ADDI") return a + b;
        if (op == "SUB")  return a - b;
        if (op == "MUL")  return a * b;
        if (op == "AND")  return a & b;
        if (op == "OR")   return a | b;
        if (op == "SLL")  return a << b;
        if (op == "SRL")  return static_cast<uint32_t>(a) >> b;
        return 0;
    }
};

inline string trim(const string &s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

inline Opcode strToOpcode(const string &s) {
    static const unordered_map<string, Opcode> table = {
        {"ADD", Opcode::ADD}, {"ADDI", Opcode::ADDI}, {"SUB", Opcode::SUB},
        {"MUL", Opcode::MUL}, {"AND", Opcode::AND}, {"OR", Opcode::OR},
        {"SLL", Opcode::SLL}, {"SRL", Opcode::SRL}, {"LW", Opcode::LW},
        {"SW", Opcode::SW}, {"BEQ", Opcode::BEQ}, {"J", Opcode::J},
        {"NOP", Opcode::NOP}
    };
    auto it = table.find(s);
    return (it != table.end()) ? it->second : Opcode::INVALID;
}

#endif 



