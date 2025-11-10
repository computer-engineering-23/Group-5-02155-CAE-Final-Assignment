#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s() - " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#elif defined DEBUG_SIMPLE
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, fmt, ##args)
#else
   #define DEBUG_PRINT(fmt, args...) /* Don't do anything on release builds */
#endif /* ifdef  DEBUG */

enum RV32_REGISTERS {
   x0 = 0,
   x1,
   x2,
   x3,
   x4,
   x5,
   x6,
   x7,
   x8,
   x9,
   x10,
   x11,
   x12,
   x13,
   x14,
   x15,
   x16,
   x17,
   x18,
   x19,
   x20,
   x21,
   x22,
   x23,
   x24,
   x25,
   x26,
   x27,
   x28,
   x29,
   x30,
   x31,

   zero = x0,
   ra = x1,
   sp = x2,
   gp = x3,
   tp = x4,
   t0 = x5,
   t1 = x6,
   t2 = x7,
   s0 = x8,
   fp = x8,
   s1 = x9,
   a0 = x10,
   a1 = x11,
   a2 = x12,
   a3 = x13,
   a4 = x14,
   a5 = x15,
   a6 = x16,
   a7 = x17,
   s2 = x18,
   s3 = x19,
   s4 = x20,
   s5 = x21,
   s6 = x22,
   s7 = x23,
   s8 = x24,
   s9 = x25,
   s10 = x26,
   s11 = x27,
   t3 = x28,
   t4 = x29,
   t5 = x30,
   t6 = x31,
};

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

   struct {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t imm : 20;
   } u_type;
} rv32_instruction_t;

typedef enum RV32OPCODES {
   R = 0b0110011, // 0x33
   I = 0b0010011, // 0x13
} rv32opcodes_t;


constexpr uint32_t memory_size = 1024 * 1024; // 1 MiB
static uint8_t *memory;

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

int main(int argc, char *argv[]) {
   int ret = 0;

   if (argc < 2) {
      fprintf(stdout, "Usage: %s <binary_file> [output_file]\n", argv[0]);
      ret = 0;
      goto file_not_specified;
   }

   memory = malloc(memory_size);
   if (memory == nullptr) {
      fprintf(stderr, "Failed to initialize memory\n");
      ret = -1;
      goto err_file_not_opened;
   }

   FILE *file = fopen(argv[1], "rb");
   if (file == nullptr) {
      fprintf(stderr, "File not found\n");
      ret = -1;
      goto err_file_empty;
   }
   // Read contents of the program file into memory
   size_t program_size = fread(memory, 1, memory_size, file);
   fclose(file);
   if (program_size == 0) {
      fprintf(stderr, "File is empty or read error occurred\n");
      ret = -1;
      goto err_file_empty;
   }

   DEBUG_PRINT("Loaded %zu bytes\n", program_size);

   for (;;) {
      rv32_instruction_t instruction;
      memcpy(&instruction, &memory[pc], sizeof(instruction));
      //rv32_instruction_t instruction = memory[pc];

      DEBUG_PRINT("PC=0x%08X, Instruction=0x%08X, Opcode=0x%02X (0b%07b)\n", pc, instruction.raw, instruction.opcode, instruction.opcode);
      switch (instruction.opcode) {
         case 0x33: // ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
         {
            uint16_t funct = (instruction.r_type.funct7 << 3) | instruction.r_type.funct3;
            switch (funct) {
               case (0x00<<3)|0x00: registers[instruction.rd] = registers[instruction.r_type.rs1] + registers[instruction.r_type.rs2]; break; // ADD
               case (0x20<<3)|0x00: registers[instruction.rd] = registers[instruction.r_type.rs1] - registers[instruction.r_type.rs2]; break; // SUB
               case (0x00<<3)|0x04: registers[instruction.rd] = registers[instruction.r_type.rs1] ^ registers[instruction.r_type.rs2]; break; // XOR
               case (0x00<<3)|0x06: registers[instruction.rd] = registers[instruction.r_type.rs1] | registers[instruction.r_type.rs2]; break; // OR
               case (0x00<<3)|0x07: registers[instruction.rd] = registers[instruction.r_type.rs1] & registers[instruction.r_type.rs2]; break; // AND
               case (0x00<<3)|0x01: registers[instruction.rd] = registers[instruction.r_type.rs1] << (registers[instruction.r_type.rs2] & 0x1F); break; // SLL
               case (0x00<<3)|0x05: registers[instruction.rd] = registers[instruction.r_type.rs1] >> (registers[instruction.r_type.rs2] & 0x1F); break; // SRL
               case (0x20<<3)|0x05: registers[instruction.rd] = (int32_t)registers[instruction.r_type.rs1] >> (registers[instruction.r_type.rs2] & 0x1F); break; // SRA
               case (0x00<<3)|0x02: registers[instruction.rd] = ((int32_t)registers[instruction.r_type.rs1] < (int32_t)registers[instruction.r_type.rs2]) ? 1 : 0; break; // SLT
               case (0x00<<3)|0x03: registers[instruction.rd] = (registers[instruction.r_type.rs1] < registers[instruction.r_type.rs2]) ? 1 : 0; break; // SLTU
            }

            break;
         }

         case 0x13: // ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
         {
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
         }

         case 0x37: // LUI
         {
            registers[instruction.rd] = (uint32_t)instruction.u_type.imm << 12; // LUI
            break;
         }

         case 0x73: // ECALL
         {
            switch (registers[a7]) {
               case 1: fprintf(stdout, "%d", (int32_t)registers[a0]); break; // print_int
               case 2: fprintf(stdout, "%f", (float)registers[a0]); break; // print_float
               case 4: fprintf(stdout, "%s", (char *)&memory[registers[a0]]); break; // print_string
               case 10: goto end; break; // exit
               case 11: fprintf(stdout, "%c", (char)registers[a0]); break; // print_char
               case 34: fprintf(stdout, "0x%02X", registers[a0]); break; // print_hex
               case 35: fprintf(stdout, "0b%08b", registers[a0]); break; // print_bin
               case 36: fprintf(stdout, "%u", registers[a0]); break; // print_unsigned
               case 93: ret = registers[a0]; goto end; break; // exit
            }
            break;
         }

         default:
         {
            fprintf(stdout, "Opcode 0x%02X not implemented\n", instruction.opcode);
            break;
         }
      }

      registers[zero] = 0;

      for (int i = 0; i < num_registers; i++) {
         DEBUG_PRINT("0x%08X ", registers[i]);
      }
      DEBUG_PRINT("\n");

      pc += 4;
      if (pc >= program_size) {
         pc = 0;
      }
   }

end:
   char *res_fname = NULL;
   if (argc > 2)
      res_fname = argv[2];

   // Write output to file if file is specified, else write to stdout
   if (res_fname == NULL) {
      for (int i = 0; i < num_registers; i++) {
         fprintf(stdout, "%02X", registers[i]);
      }
      fprintf(stdout, "\n");
   }
   else {
      FILE *res_file = fopen(res_fname, "wb");
      fwrite(registers, 4, num_registers, res_file);
      fclose(res_file);
   }

err_file_not_opened:
err_file_empty:
   free(memory);

file_not_specified:
   return ret;
}

