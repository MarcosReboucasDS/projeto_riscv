#include <iostream> //PARA IMPRIMIR NO TERMINAL
#include <vector> //Para armazenar os registradores
#include <cstdint> //Para usar tipos como uint32_t (32 bits)
#include <tuple> //Para retornar vários valores no decode
// Define opcodes for RV32I instructions
enum OPCODES { //aqui mostra todos os opcodes, no caso, quais as operações de fazer
    LUI = 0b0110111, //instruções do tipo U (LUI, AUIPC)
    AUIPC = 0b0010111, 
    JAL = 0b1101111, //instruções de salto (JAL, JALR)
    JALR = 0b1100111,
    BRANCH = 0b1100011, //instruções de desvio (BRANCH)
    LOAD = 0b0000011, //loads/stores
    STORE = 0b0100011,
    ALU_IMM = 0b0010011, //ALU imediato (ALU_IMM)
    ALU_REG = 0b0110011, //ALU registrador-registrador (ALU_REG)
};
// Define function codes for ALU operations
enum FUNCT3_CODES {  // define a função para operações da ALU conforme o campo funct3
    ADD_SUB = 0b000, //add ou sub
    SLL = 0b001, //shift lógico
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
        pc = 0;                   // Inicializa o programa
        memory.resize(memory_size, 0);  // Inicia a memória com um tamanho específico
    }
    void load_program(const std::vector<uint32_t>& program) {
        for (size_t i = 0; i < program.size(); ++i) {
            memory[i] = program[i];  // Carrega o programa na memória
        }
    }
    void run(){
        while(pc < memory.size()) {
            uint32_t instruction = fetch();
            auto [opcode, rd, funct3, rs1, rs2, funct7, imm] = decode(instruction);
            execute(instruction, opcode, rd, funct3, rs1, rs2, funct7, imm);
        }
    }
    std::vector<uint32_t> registers;
private:
    std::vector<uint32_t> memory;
    uint32_t pc;
    // Recupera a próxima instrução da memória
    uint32_t fetch() {
        return memory[pc++];
    }
    // Decodifica a instrução obtida.
    std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int32_t>
    decode(uint32_t instruction) {
        uint32_t opcode = instruction & 0x7F;
        uint32_t rd = (instruction >> 7) & 0x1F;
        uint32_t funct3 = (instruction >> 12) & 0x7;
        uint32_t rs1 = (instruction >> 15) & 0x1F;
        uint32_t rs2 = (instruction >> 20) & 0x1F;
        uint32_t funct7 = (instruction >> 25) & 0x7F;
        int32_t imm;
        uint32_t imm_12 = (instruction >> 20) & 0xFFF;
        // extensão de sinal de 12 bits
        if (imm_12 & 0x800)
            imm = imm_12 | 0xFFFFF000;
        else
            imm = imm_12;

        return {opcode, rd, funct3, rs1, rs2, funct7, imm};
    }
    // executa a instrução decodificada
    void execute(uint32_t instruction, uint32_t opcode, uint32_t rd,
             uint32_t funct3, uint32_t rs1, uint32_t rs2,
             uint32_t funct7, int32_t imm){
        switch (opcode) {
            case LUI:
                registers[rd] = imm << 12;
                break;
            case AUIPC:
                registers[rd] = pc + (imm << 12);
                break;
            case JAL: {
                // extrair J-type immediate (em bytes)
                int32_t imm_j = 0;
                imm_j |= ((instruction >> 31) & 0x1) << 20;   // imm[20] = inst[31]
                imm_j |= ((instruction >> 21) & 0x3FF) << 1;  // imm[10:1] = inst[30:21]
                imm_j |= ((instruction >> 20) & 0x1) << 11;   // imm[11] = inst[20]
                imm_j |= ((instruction >> 12) & 0xFF) << 12;  // imm[19:12] = inst[19:12]
                imm_j = (imm_j << 11) >> 11; // sign-extend 21 bits

                registers[rd] = pc * 4u; // SALVA endereço de retorno em BYTES 
                // calcula índice da instrução alvo:
                int32_t instr_offset = imm_j / 4; // imm_j é múltiplo de 4, então ok
                pc = (pc - 1) + instr_offset;     // define pc para o índice de instrução alvo
                break;
            }
            case JALR: {
                // JALR usa I-type imm (em bytes). Comportamento:
                // t = (registers[rs1] + imm) & ~1; // endereço em bytes
                // pc := t / 4  (converte byte address para índice de instrução)
                registers[rd] = pc * 4u; // salva endereço de retorno em bytes           
                uint32_t target_byte = (uint32_t)(registers[rs1] + imm) & ~1u; // limpa LSB
                pc = target_byte >> 2; // converte byte address para índice de instrução
                break;
            }
            case ALU_IMM:
                switch (funct3) {
                    case ADD_SUB:  // ADDI
                        registers[rd] = registers[rs1] + imm;
                        break;
            
                    case SLL:  // SLLI
                        registers[rd] = registers[rs1] << (imm & 0x1F);
                        break;
            
                    case SLT:  // SLTI
                        registers[rd] = ((int32_t)registers[rs1] < (int32_t)imm);
                        break;
            
                    case SLTU:  // SLTIU
                        registers[rd] = (registers[rs1] < (uint32_t)imm);
                        break;
            
                    case XOR:  // XORI
                        registers[rd] = registers[rs1] ^ imm;
                        break;
            
                    case SRL_SRA:
                        if ((instruction >> 30) & 1) // SRAI
                            registers[rd] = ((int32_t)registers[rs1]) >> (imm & 0x1F);
                        else // SRLI
                            registers[rd] = registers[rs1] >> (imm & 0x1F);
                        break;
            
                    case OR: { // ORI
                        uint32_t uimm = imm & 0xFFF;  // zero-extend de 12 bits
                        registers[rd] = registers[rs1] | uimm;
                        break;
                    }
                    
                    case AND: { // ANDI
                        uint32_t uimm = imm & 0xFFF;
                        registers[rd] = registers[rs1] & uimm;
                        break;
                    }

                }
                break;

            case ALU_REG:
                switch (funct3)
                {
                    case ADD_SUB:
                        registers[rd] = (funct7 == 0x20)
                            ? registers[rs1] - registers[rs2]   // SUB
                            : registers[rs1] + registers[rs2];  // ADD
                        break;
            
                    case SLL:
                        registers[rd] = registers[rs1] << (registers[rs2] & 0x1F);
                        break;
            
                    case SLT:
                        registers[rd] = ((int32_t)registers[rs1] < (int32_t)registers[rs2]);
                        break;
            
                    case SLTU:
                        registers[rd] = (registers[rs1] < registers[rs2]);
                        break;
            
                    case XOR:
                        registers[rd] = registers[rs1] ^ registers[rs2];
                        break;
            
                    case SRL_SRA:
                        if (funct7 == 0x20) // SRA
                            registers[rd] = ((int32_t)registers[rs1]) >> (registers[rs2] & 0x1F);
                        else // SRL
                            registers[rd] = registers[rs1] >> (registers[rs2] & 0x1F);
                        break;
            
                    case OR:
                        registers[rd] = registers[rs1] | registers[rs2];
                        break;
            
                    case AND:
                        registers[rd] = registers[rs1] & registers[rs2];
                        break;
                }
                break;
    
            case BRANCH: {
                int32_t imm_b = 0;
            
                imm_b |= ((instruction >> 31) & 1) << 12;     // imm[12]
                imm_b |= ((instruction >> 25) & 0x3F) << 5;   // imm[10:5]
                imm_b |= ((instruction >> 8) & 0xF) << 1;     // imm[4:1]
                imm_b |= ((instruction >> 7) & 1) << 11;      // imm[11]
            
                // sign-extend 13 bits
                imm_b = (imm_b << 19) >> 19;
            
                int32_t instr_offset = imm_b / 4;  // converter bytes → instruções
            
                switch (funct3) {
                    case 0b000: if (registers[rs1] == registers[rs2]) pc += instr_offset - 1; break; // BEQ
                    case 0b001: if (registers[rs1] != registers[rs2]) pc += instr_offset - 1; break; // BNE
                    case 0b100: if ((int32_t)registers[rs1] <  (int32_t)registers[rs2]) pc += instr_offset - 1; break;
                    case 0b101: if ((int32_t)registers[rs1] >= (int32_t)registers[rs2]) pc += instr_offset - 1; break;
                    case 0b110: if (registers[rs1] <  registers[rs2]) pc += instr_offset - 1; break;
                    case 0b111: if (registers[rs1] >= registers[rs2]) pc += instr_offset - 1; break;
                }
            
                break;
            }

    

            // Caso haver mais instruções, colocar aqui
        }
    
    registers[0] = 0;             
    }
};

 void run_test(const std::string& name,
              const std::vector<uint32_t>& program,
              const std::vector<int32_t>& expected_values,
              const std::vector<int> reg_ids){ //função de teste
        RiscVEmulator emu;
        emu.load_program(program);
        emu.run();

        std::cout << "\n=== TESTE: " << name << " ===\n";
    
        for (size_t i = 0; i < reg_ids.size(); i++) {
            int r = reg_ids[i];
            int32_t value = emu.registers[r];
            int32_t expected = expected_values[i];
    
            std::cout << "x" << r << " = " << value
                      << " (esperado " << expected << ")";
    
            if (value == expected) std::cout << "  ✔\n";
            else                   std::cout << "  ✘\n";
        }
    
        std::cout << "-----------------------------\n";
    }

   

int main() {
    //ARITMÉTICA
    run_test("ADD / SUB / ADDI",{
        0x00500093,  // ADDI x1, x0, 5
        0x00308133,  // ADD  x2, x1, x3 (x3=0) => 5
        0x401101b3,  // SUB  x3, x2, x1 => 0
    },
    {5, 5, 0},
    {1, 2, 3});
    
    //LÓGICA

    // XOR / XORI
    run_test("XOR / XORI",
    {
        0x00F00093,   // ADDI x1, x0, 15   (x1 = 15)
        0x00300113,   // ADDI x2, x0, 3    (x2 = 3)
    
        0x0020C1B3,   // XOR  x3, x1, x2 => 12
        0x0040C213    // XORI x4, x1, 4   => 11
    },
    {
        15, 3,  // x1, x2 (opcionais para conferência)
        12, 11  // x3 (XOR), x4 (XORI)
    },
    {1,2,3,4});
    
    
    // OR / ORI
    run_test("OR / ORI",
    {
        0x00F00093,   // ADDI x1, x0, 15
        0x00300113,   // ADDI x2, x0, 3
    
        0x020E1B3,    // OR   x3, x1, x2 => 15
        0x0040E213     // ORI  x4, x1, 4  => 15
    },
    {
        15, 3,  // x1, x2
        15, 15  // x3 (OR), x4 (ORI)
    },
    {1,2,3,4});
    
    
    // AND / ANDI
    run_test("AND / ANDI",
    {
        0x00F00093,   // ADDI x1, x0, 15
        0x00300113,   // ADDI x2, x0, 3
    
        0x020F1B3,    // AND  x3, x1, x2 => 3
        0x0040F213     // ANDI x4, x1, 4  => 4
    },
    {
        15, 3,  // x1, x2
        3, 4    // x3 (AND), x4 (ANDI)
    },
    {1,2,3,4});

    
    //COMPARAÇÃO
    run_test("SLT / SLTI / SLTU / SLTIU",
    {
        0xFFF00093,   // ADDI x1, x0, -1
        0x00100113,   // ADDI x2, x0, 1
    
        0x0020A1B3,   // SLT  x3, x1, x2  => 1
        0x0020B213,   // SLTU x4, x1, x2 => 0
    
        0x0010A293,   // SLTI x5, x1, 1   => 1
        0x0010B313,   // SLTIU x6, x1, 1  => 0
    },
    { -1, 1, 1, 0, 1, 0 },
    {1,2,3,4,5,6});
    
    //DESVIOS
    run_test("BEQ / BNE / BLT / BGE / BLTU / BGEU",
    {
        0x00500093,  // ADDI x1, x0, 5
        0x00500113,  // ADDI x2, x0, 5
        0x00600193,  // ADDI x3, x0, 6
    
        0x00210663,  // BEQ x1,x2,+12 → pula
        0x00100213,  // ADDI x4, x0, 1 
    },
    {5,5,6,0},
    {1,2,3,4});

    
    //JUMPS
    run_test("JAL / JALR",
    {
        0x008000EF,   // JAL x1, 8    → pc avança e salva retorno
        0x00000013,   // NOP (pulado)
        0x00100113,   // ADDI x2, x0, 1
    },
    {4,1},
    {1,2});
}
//https://medium.com/@techAsthetic/building-a-simple-risc-v-emulator-in-c-b4a4c914cb93