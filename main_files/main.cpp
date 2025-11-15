#include "mips_ir.hpp"
#include "mips_pipeline.cpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

// Helper function to trim whitespace
inline string trim(const string& s) {
    auto start = s.find_first_not_of(" \t");
    auto end = s.find_last_not_of(" \t");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

// Helper function to parse instruction from string directly to IR format
Instruction parseInstruction(const string& line) {
    Instruction instr;
    string trimmed = trim(line);
    
    if (trimmed.empty() || trimmed[0] == '#') {
        instr.op = Op::NOP;
        return instr;
    }
    
    istringstream iss(trimmed);
    string opcodeStr;
    iss >> opcodeStr;
    
    // Convert to uppercase
    transform(opcodeStr.begin(), opcodeStr.end(), opcodeStr.begin(), ::toupper);
    
    // Map opcode string to Op enum
    if (opcodeStr == "ADD") instr.op = Op::ADD;
    else if (opcodeStr == "ADDI") instr.op = Op::ADDI;
    else if (opcodeStr == "SUB") instr.op = Op::SUB;
    else if (opcodeStr == "MUL") instr.op = Op::MUL;
    else if (opcodeStr == "AND") instr.op = Op::AND;
    else if (opcodeStr == "OR") instr.op = Op::OR;
    else if (opcodeStr == "SLL") instr.op = Op::SLL;
    else if (opcodeStr == "SRL") instr.op = Op::SRL;
    else if (opcodeStr == "LW") instr.op = Op::LW;
    else if (opcodeStr == "SW") instr.op = Op::SW;
    else if (opcodeStr == "BEQ") instr.op = Op::BEQ;
    else if (opcodeStr == "J") instr.op = Op::J;
    else {
        instr.op = Op::NOP;
        return instr;
    }
    
    // Parse operands based on instruction type
    char comma, paren;
    string regStr;
    
    switch (instr.op) {
        case Op::ADD:
        case Op::SUB:
        case Op::MUL:
        case Op::AND:
        case Op::OR:
            // R-type: ADD rd, rs, rt
            if (iss >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rd = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rs = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rt = static_cast<uint8_t>(stoi(regStr));
            }
            break;
            
        case Op::SLL:
        case Op::SRL:
            // R-type shift: SLL rd, rt, shamt
            if (iss >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rd = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rt = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma) {
                iss >> instr.imm; // shamt
            }
            break;
            
        case Op::ADDI:
        case Op::LW:
        case Op::SW:
            // I-type: ADDI rt, rs, imm  or  LW rt, imm(rs)  or  SW rt, imm(rs)
            if (iss >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rt = static_cast<uint8_t>(stoi(regStr));
            }
            if (instr.op == Op::LW || instr.op == Op::SW) {
                // Format: LW rt, imm(rs)
                if (iss >> instr.imm >> paren >> regStr) {
                    if (regStr[0] == '$') regStr = regStr.substr(1);
                    instr.rs = static_cast<uint8_t>(stoi(regStr));
                }
            } else {
                // Format: ADDI rt, rs, imm
                if (iss >> comma >> regStr) {
                    if (regStr[0] == '$') regStr = regStr.substr(1);
                    instr.rs = static_cast<uint8_t>(stoi(regStr));
                }
                if (iss >> comma) {
                    iss >> instr.imm;
                }
            }
            break;
            
        case Op::BEQ:
            // I-type: BEQ rs, rt, label (we'll use immediate as offset)
            if (iss >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rs = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma >> regStr) {
                if (regStr[0] == '$') regStr = regStr.substr(1);
                instr.rt = static_cast<uint8_t>(stoi(regStr));
            }
            if (iss >> comma) {
                iss >> instr.imm;
            }
            break;
            
        case Op::J:
            // J-type: J address
            iss >> instr.addr;
            break;
            
        case Op::NOP:
        default:
            break;
    }
    
    return instr;
}

// Using MIPSPipeline from mips_pipeline.cpp instead of SimplePipeline

int main(int argc, char* argv[]) {
    vector<Instruction> program;
    
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
    
    // Output results
    cout << "\n" << string(60, '=') << endl;
    cout << " FINAL REGISTER FILE" << endl;
    cout << string(60, '=') << endl;
    
    const auto& regs = pipeline.regs();
    for (int i = 0; i < 32; i += 4) {
        cout << left << setw(8) << "Reg"
             << setw(8) << "Name"
             << setw(12) << "Decimal"
             << setw(12) << "Hex" << endl;
        
        for (int j = i; j < min(i + 4, 32); ++j) {
            int32_t value = regs[j];
            const vector<string> regNames = {
                "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
                "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
                "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
            };
            string name = (j < regNames.size()) ? regNames[j] : "???";
            
            cout << left << setw(8) << ("$" + to_string(j))
                 << setw(8) << name
                 << setw(12) << dec << value
                 << "0x" << hex << setw(8) << setfill('0')
                 << static_cast<uint32_t>(value)
                 << dec << setfill(' ') << endl;
        }
        cout << endl;
    }
    cout << string(60, '=') << endl;
    
    cout << "\n" << string(60, '=') << endl;
    cout << " FINAL MEMORY CONTENTS" << endl;
    cout << string(60, '=') << endl;
    cout << "Memory (showing first 256 bytes, address 0x00000000 - 0x000000FF):\n";
    
    const auto& mem = pipeline.memory();
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
    
    cout << "\nTotal cycles: " << pipeline.cycles() << endl;
    
    return 0;
}
