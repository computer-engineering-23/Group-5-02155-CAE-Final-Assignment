#include <stdint.h>
#include <stdio.h>

typedef union {
   int16_t immediatei;
   uint16_t immediateu;
   uint8_t  rs1;
   uint8_t  rs2;
   uint8_t  funct3;
   uint8_t  rd;
   uint8_t  opcode;
   uint8_t  funct7;
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
      uint32_t instruction_raw = program[pc >> 2];
      rv32i_instruction_t instruction = {
         .immediatei = sign_extend(get_bits(instruction_raw, 31, 20), 12),
         .immediateu = get_bits(instruction_raw, 31, 20),
         .rs1 = (instruction_raw >> 15) & 0x01F,
         .rs2 = get_bits(instruction_raw, 24, 20),
         .rd = (instruction_raw >> 7) & 0x01F,
         .opcode = instruction_raw & 0x7F,
         .funct3 = get_bits(instruction_raw, 14, 12),
         .funct7 = get_bits(instruction_raw, 31, 25),
      };

      printf("0x%02X\n", instruction.raw);
      switch (instruction.opcode) {
         case 0x33: // ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
            switch ((instruction.funct7 << 3) | instruction.funct3) {
               case (0x00<<3)|0x00: registers[instruction.rd] = registers[instruction.rs1] + registers[instruction.rs2]; break; // ADD
               case (0x20<<3)|0x00: registers[instruction.rd] = registers[instruction.rs1] - registers[instruction.rs2]; break; // SUB
               case (0x00<<3)|0x04: registers[instruction.rd] = registers[instruction.rs1] ^ registers[instruction.rs2]; break; // XOR
               case (0x00<<3)|0x06: registers[instruction.rd] = registers[instruction.rs1] | registers[instruction.rs2]; break; // OR
               case (0x00<<3)|0x07: registers[instruction.rd] = registers[instruction.rs1] & registers[instruction.rs2]; break; // AND
               case (0x00<<3)|0x01: registers[instruction.rd] = registers[instruction.rs1] << registers[instruction.rs2]; break; // SLL
               case (0x00<<3)|0x05: registers[instruction.rd] = registers[instruction.rs1] >> registers[instruction.rs2]; break; // SLR
               case (0x20<<3)|0x05: registers[instruction.rd] = (int32_t)registers[instruction.rs1] << registers[instruction.rs2]; break; // SRA
               case (0x00<<3)|0x02: registers[instruction.rd] = (registers[instruction.rs1] < registers[instruction.rs2]) ? 1 : 0; break; // SLT
               case (0x00<<3)|0x03: registers[instruction.rd] = (registers[instruction.rs1] < registers[instruction.rs2]) ? 1 : 0; break; // SLTU
            }

            break;

         case 0x13: // ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
            switch (instruction.funct3) {
               case 0x00: registers[instruction.rd] = registers[instruction.rs1] + instruction.immediatei; break; // ADDI
               case 0x04: registers[instruction.rd] = registers[instruction.rs1] ^ instruction.immediatei; break; // XORI
               case 0x06: registers[instruction.rd] = registers[instruction.rs1] | instruction.immediatei; break; // ORI
               case 0x07: registers[instruction.rd] = registers[instruction.rs1] & instruction.immediatei; break; // ANDI
               case 0x01: registers[instruction.rd] = registers[instruction.rs1] << (instruction.immediatei & 0b11111); break; // SSLI
               case 0x05: {
                  if (get_bits(instruction.immediatei, 11, 5) == 0x00) { // SRLI
                     registers[instruction.rd] = registers[instruction.rs1] >> get_bits(instruction.immediatei, 0, 4);
                  }
                  else { // SRLAI
                     registers[instruction.rd] = (int32_t)registers[instruction.rs1] >> get_bits(instruction.immediatei, 0, 4);
                  }
               }
               case 0x02: registers[instruction.rd] = (registers[instruction.rs1] < instruction.immediatei) ? 1 : 0; break; // SLTI
               case 0x03: registers[instruction.rd] = ((uint32_t)registers[instruction.rs1] < instruction.immediatei) ? 1 : 0; break; // SLTIU
            }
            break;

         default:
            printf("Opcode 0x%02X not implemented\n", instruction.opcode);
            break;
      }


      for (int i = 0; i < num_registers; i++) {
         printf("0x%02X ", registers[i]);
      }
      printf("\n");

      pc += 4;
      if ((pc >> 2) >= program_size) {
         break;
      }
   }
}
