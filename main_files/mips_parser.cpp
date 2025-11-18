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
    DecodedInstruction decoded;
    
    // Extract opcode (bits 31-26)
    decoded.opcode = (instruction >> 26) & 0x3F;
	
	if (decoded.opcode == 0) {
    decoded.type = R_TYPE;
    decoded.rs = (instruction >> 21) & 0x1F;    // bits 25-21
    decoded.rt = (instruction >> 16) & 0x1F;    // bits 20-16
    decoded.rd = (instruction >> 11) & 0x1F;    // bits 15-11
    decoded.shamt = (instruction >> 6) & 0x1F;  // bits 10-6
    decoded.funct = instruction & 0x3F;          // bits 5-0
	}
	else if (decoded.opcode != 2 && decoded.opcode != 3) {  
    decoded.type = I_TYPE;
    decoded.rs = (instruction >> 21) & 0x1F;    // bits 25-21
    decoded.rt = (instruction >> 16) & 0x1F;    // bits 20-16
    decoded.immediate = instruction & 0xFFFF;  	// bits 15-0
	}
	else if (decoded.opcode == 2 || decoded.opcode == 3) { 
    decoded.type = J_TYPE;
    decoded.address = instruction & 0x3FFFFFF;   // bits 25-0
	}
	return decoded;
}




