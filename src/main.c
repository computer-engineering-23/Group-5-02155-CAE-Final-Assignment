#include <stdint.h>
#include <stdio.h>

typedef union {
   uint32_t raw; // Raw 32-bit instruction

   // These fields are universal
   struct {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t __unused : 20;
   };

   struct {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t rs2 : 5;
      uint32_t funct7 : 7;
   } r_type;

   struct {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t imm : 12;
   } i_type;

   struct {
      uint32_t opcode : 7;
      uint32_t imm_low : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t rs2 : 5;
      uint32_t imm_high : 7;
   } s_type;

} rv32_instruction_t;

typedef enum RV32OPCODES {
   R = 0b0110011, // 0x33
   I = 0b0010011, // 0x13
} rv32opcodes_t;


constexpr uint32_t program[] = {
   // As minimal RISC-V assembler example
   0x00200093, // addi x1 x0 2
   0x00300113, // addi x2 x0 3
   0x002081b3, // add x3 x1 x2
};
constexpr uint32_t program_size = sizeof(program) / sizeof(program[0]);

uint32_t pc = 0;
constexpr uint8_t num_registers = 32;
uint32_t registers[num_registers] = {0};

static inline uint32_t get_bits(uint32_t x, int high, int low) {
   return (x >> low) & ((1u << (high - low + 1)) - 1u);
}

static inline int32_t sign_extend(uint32_t val, int bits) {
   uint32_t m = 1 << (bits - 1);
   return (int32_t)((val^m) - m);
}

int main() {

   for (;;) {
      rv32_instruction_t instruction;
      instruction.raw = program[pc >> 2];

      printf("PC=0x%08X, Instruction=0x%08X, Opcode=0x%02X\n", pc, instruction.raw, instruction.opcode);
      switch (instruction.opcode) {
         case 0x33: // ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
            uint16_t funct = (instruction.r_type.funct7 << 3) | instruction.r_type.funct3;
            switch (funct) {
               case (0x00<<3)|0x00: registers[instruction.rd] = registers[instruction.r_type.rs1] + registers[instruction.r_type.rs2]; break; // ADD
               case (0x20<<3)|0x00: registers[instruction.rd] = registers[instruction.r_type.rs1] - registers[instruction.r_type.rs2]; break; // SUB
               case (0x00<<3)|0x04: registers[instruction.rd] = registers[instruction.r_type.rs1] ^ registers[instruction.r_type.rs2]; break; // XOR
               case (0x00<<3)|0x06: registers[instruction.rd] = registers[instruction.r_type.rs1] | registers[instruction.r_type.rs2]; break; // OR
               case (0x00<<3)|0x07: registers[instruction.rd] = registers[instruction.r_type.rs1] & registers[instruction.r_type.rs2]; break; // AND
               case (0x00<<3)|0x01: registers[instruction.rd] = registers[instruction.r_type.rs1] << (registers[instruction.r_type.rs2] & 0x1F); break; // SLL
               case (0x00<<3)|0x05: registers[instruction.rd] = registers[instruction.r_type.rs1] >> registers[instruction.r_type.rs2]; break; // SRL
               case (0x20<<3)|0x05: registers[instruction.rd] = (int32_t)registers[instruction.r_type.rs1] >> registers[instruction.r_type.rs2]; break; // SRA
               case (0x00<<3)|0x02: registers[instruction.rd] = ((int32_t)registers[instruction.r_type.rs1] < (int32_t)registers[instruction.r_type.rs2]) ? 1 : 0; break; // SLT
               case (0x00<<3)|0x03: registers[instruction.rd] = (registers[instruction.r_type.rs1] < registers[instruction.r_type.rs2]) ? 1 : 0; break; // SLTU
            }

            break;

         case 0x13: // ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
            int32_t imm = sign_extend(instruction.i_type.imm, 12);
            switch (instruction.i_type.funct3) {
               case 0x00: registers[instruction.rd] = registers[instruction.i_type.rs1] + imm; break; // ADDI
               case 0x04: registers[instruction.rd] = registers[instruction.i_type.rs1] ^ imm; break; // XORI
               case 0x06: registers[instruction.rd] = registers[instruction.i_type.rs1] | imm; break; // ORI
               case 0x07: registers[instruction.rd] = registers[instruction.i_type.rs1] & imm; break; // ANDI
               case 0x01: registers[instruction.rd] = registers[instruction.i_type.rs1] << (imm & 0x1F); break; // SSLI
               case 0x05: {
                  uint8_t shift_amount = instruction.i_type.imm & 0x1F;  // How many positions to shift
                  uint8_t mode = get_bits(instruction.i_type.imm, 11, 5);
                  if (mode == 0x00) { // SRLI
                     registers[instruction.rd] = registers[instruction.i_type.rs1] >> shift_amount;
                  }
                  else { // SRLAI
                     registers[instruction.rd] = (int32_t)registers[instruction.i_type.rs1] >> shift_amount;
                  }
                  break;
               }
               case 0x02: registers[instruction.rd] = ((int32_t)registers[instruction.i_type.rs1] < imm) ? 1 : 0; break; // SLTI
               case 0x03: registers[instruction.rd] = (registers[instruction.i_type.rs1] < (uint32_t)imm) ? 1 : 0; break; // SLTIU
            }
            break;

         default:
            printf("Opcode 0x%02X not implemented\n", instruction.opcode);
            break;
      }

      registers[0] = 0;

      for (int i = 0; i < num_registers; i++) {
         printf("0x%08X ", registers[i]);
      }
      printf("\n");

      pc += 4;
      if ((pc >> 2) >= program_size) {
         break;
      }
   }
}

