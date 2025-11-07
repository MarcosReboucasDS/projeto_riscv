// Type your code here, or load an example.
#include <iostream>
#include <vector>
#include <cstdint>
#include <tuple>
// Define opcodes for RV32I instructions
enum OPCODES { //aqui mostra todos os opcodes, no caso, quais as operações de fazer
    LUI = 0b0110111,
    AUIPC = 0b0010111,
    JAL = 0b1101111,
    JALR = 0b1100111,
    BRANCH = 0b1100011,
    LOAD = 0b0000011,
    STORE = 0b0100011,
    ALU_IMM = 0b0010011,
    ALU_REG = 0b0110011,
};
// Define function codes for ALU operations
enum FUNCT3_CODES {  //define a função para 
    ADD_SUB = 0b000,
    SLL = 0b001,
    SLT = 0b010,
    SLTU = 0b011,
    XOR = 0b100,
    SRL_SRA = 0b101,
    OR = 0b110,
    AND = 0b111,
};

class RiscVEmulator {
public:
    RiscVEmulator(size_t memory_size = 1024) {
        registers.resize(32, 0);  // 32 general-purpose registers
        pc = 0;                   // Initialize the program counter
        memory.resize(memory_size, 0);  // Initialize memory with a given size
    }
void load_program(const std::vector<uint32_t>& program) {
        for (size_t i = 0; i < program.size(); ++i) {
            memory[i] = program[i];  // Load the program into memory
        }
    }
    void run() {
        while (pc < memory.size()) {
            uint32_t instruction = fetch();
            auto [opcode, rd, funct3, rs1, rs2, funct7, imm] = decode(instruction);
            execute(opcode, rd, funct3, rs1, rs2, funct7, imm);
        }
    }
    std::vector<uint32_t> registers;
private:
    std::vector<uint32_t> memory;
    uint32_t pc;
    // Fetch the next instruction from memory
    uint32_t fetch() {
        return memory[pc++];
    }
    // Decode the fetched instruction
    std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int32_t>
    decode(uint32_t instruction) {
        uint32_t opcode = instruction & 0x7F;
        uint32_t rd = (instruction >> 7) & 0x1F;
        uint32_t funct3 = (instruction >> 12) & 0x7;
        uint32_t rs1 = (instruction >> 15) & 0x1F;
        uint32_t rs2 = (instruction >> 20) & 0x1F;
        uint32_t funct7 = (instruction >> 25) & 0x7F;
        int32_t imm = static_cast<int32_t>(instruction) >> 20;  // Sign-extend the immediate value
        return {opcode, rd, funct3, rs1, rs2, funct7, imm};
    }
    // Execute the decoded instruction
    void execute(uint32_t opcode, uint32_t rd, uint32_t funct3, uint32_t rs1, uint32_t rs2, uint32_t funct7, int32_t imm) {
        switch (opcode) {
            case LUI:
                registers[rd] = imm << 12;
                break;
            case AUIPC:
                registers[rd] = pc + (imm << 12);
                break;
            case JAL:
                registers[rd] = pc;
                pc += imm;
                break;
            case JALR:
                registers[rd] = pc;
                pc = (registers[rs1] + imm) & ~1;
                break;
            case ALU_IMM:
                if (funct3 == ADD_SUB) {
                    registers[rd] = registers[rs1] + imm;
                }
                break;
            case ALU_REG:
                if (funct3 == ADD_SUB) {
                    if (funct7 == 0b0000000) {
                        registers[rd] = registers[rs1] + registers[rs2];
                    } else if (funct7 == 0b0100000) {
                        registers[rd] = registers[rs1] - registers[rs2];
                    }
                }
                break;
            // Handle more instructions here...
        }
    }
};

int main() {
    std::vector<uint32_t> program = {
        0x00000013,  // NOP (ADDI x0, x0, 0)
        0x00500113,  // ADDI x2, x0, 5
        0x00600193,  // ADDI x3, x0, 6
        0x003101B3,  // ADD x3, x2, x3
    };

RiscVEmulator emulator;
    emulator.load_program(program);
    emulator.run();
    std::cout << "Result in x3: " << emulator.registers[3] << std::endl;  // Should print 11
    return 0;
}
//https://medium.com/@techAsthetic/building-a-simple-risc-v-emulator-in-c-b4a4c914cb93