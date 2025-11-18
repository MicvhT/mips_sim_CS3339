//main.cpp
#include "mips_ir.hpp"
#include "mips_core.h"
#include "mips_pipeline.h"
#include "mips_output.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <cctype>

using namespace std;

Instruction parseInstruction(const string& line) {
    Instruction instr{};
    string trimmed = trim(line);

    if (trimmed.empty() || trimmed[0] == '#') {
        instr.op = Op::NOP;
        return instr;
    }

    istringstream iss(trimmed);
    string token;
    iss >> token;

    transform(token.begin(), token.end(), token.begin(), ::toupper);

    static const unordered_map<string, Op> opMap = {
        {"ADD", Op::ADD}, {"ADDI", Op::ADDI}, {"SUB", Op::SUB}, {"MUL", Op::MUL},
        {"AND", Op::AND}, {"OR", Op::OR}, {"SLL", Op::SLL}, {"SRL", Op::SRL},
        {"LW", Op::LW}, {"SW", Op::SW}, {"BEQ", Op::BEQ}, {"J", Op::J},
        {"HALT", Op::HALT}, {"NOP", Op::NOP}
    };

    auto it = opMap.find(token);
    if (it == opMap.end()) {
        cerr << "Unknown instruction: " << token << endl;
        instr.op = Op::NOP;
        return instr;
    }
    instr.op = it->second;

    string reg;
    char comma = ',', paren;

    switch (instr.op) {
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::AND: case Op::OR:
            iss >> reg >> comma >> reg >> comma >> reg;
            instr.rd = stoi(reg.substr(reg.find('$') + 1));
            instr.rs = stoi(reg.substr(reg.find('$') + 1));
            instr.rt = stoi(reg.substr(reg.find('$') + 1));
            break;

        case Op::SLL: case Op::SRL:
            iss >> reg >> comma >> reg >> comma >> instr.shamt;
            instr.rd = stoi(reg.substr(reg.find('$') + 1));
            instr.rt = stoi(reg.substr(reg.find('$') + 1));
            break;

        case Op::ADDI:
            iss >> reg >> comma >> reg >> comma >> instr.imm;
            instr.rt = stoi(reg.substr(reg.find('$') + 1));
            instr.rs = stoi(reg.substr(reg.find('$') + 1));
            break;

        case Op::LW: case Op::SW: {
            string temp;
            iss >> reg >> comma >> temp;
            instr.rt = stoi(reg.substr(reg.find('$') + 1));
            size_t open = temp.find('(');
            string offset = temp.substr(0, open);
            string rs_str = temp.substr(open + 1, temp.find(')') - open - 1);
            instr.imm = stoi(offset);
            instr.rs = stoi(rs_str.substr(rs_str.find('$') + 1));
            break;
        }

        case Op::BEQ:
            iss >> reg >> comma >> reg >> comma;
            instr.rs = stoi(reg.substr(reg.find('$') + 1));
            instr.rt = stoi(reg.substr(reg.find('$') + 1));
            string label;
            iss >> label;
            instr.raw_label = label;
            break;

        case Op::J:
            iss >> instr.addr;
            break;

        case Op::HALT: case Op::NOP:
            break;

        default:
            break;
    }
    return instr;
}

int main(int argc, char* argv[]) {
    vector<Instruction> program;
    ifstream file;
    istream* input = &cin;

    if (argc > 1) {
        file.open(argv[1]);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << argv[1] << endl;
            return 1;
        }
        input = &file;
    }

    string line;
    while (getline(*input, line)) {
        Instruction instr = parseInstruction(line);
        if (instr.op != Op::NOP || !trim(line).empty()) {
            program.push_back(instr);
        }
    }
    if (file.is_open()) file.close();

    if (program.empty()) {
        cout << "No instructions loaded.\n";
        return 0;
    }

    MIPSPipeline pipeline(program, 1 << 16, false);
    pipeline.run();

    OutputManager output;
    output.printFinalState(pipeline.regs_, pipeline.mem_);

    cout << "\nSimulation completed in " << pipeline.cycles() << " cycles.\n";

    return 0;
}

