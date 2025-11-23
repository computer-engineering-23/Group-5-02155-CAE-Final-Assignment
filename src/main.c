#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rv32_types.h"

#ifdef DEBUG
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s() - " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
   #define DEBUG_PRINT_SIMPLE(fmt, args...) fprintf(stderr, fmt, ##args)
#else
   #define DEBUG_PRINT(fmt, args...) /* Don't do anything on release builds */
   #define DEBUG_PRINT_SIMPLE(fmt, args...) /* Don't do anything on release builds */
#endif /* ifdef  DEBUG */

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

      DEBUG_PRINT("PC=0x%08X, Instruction=0x%08X, Opcode=0x%02X (0b%07b)\n", pc, instruction.raw, instruction.opcode, instruction.opcode);
      switch (instruction.opcode) {
         case OP_REG: // ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
         {
            uint16_t funct = (instruction.r_type.funct7 << 3) | instruction.r_type.funct3;
            switch (funct) {
               case ADD:  registers[instruction.rd] = registers[instruction.r_type.rs1] + registers[instruction.r_type.rs2]; break;
               case SUB:  registers[instruction.rd] = registers[instruction.r_type.rs1] - registers[instruction.r_type.rs2]; break;
               case XOR:  registers[instruction.rd] = registers[instruction.r_type.rs1] ^ registers[instruction.r_type.rs2]; break;
               case OR:   registers[instruction.rd] = registers[instruction.r_type.rs1] | registers[instruction.r_type.rs2]; break;
               case AND:  registers[instruction.rd] = registers[instruction.r_type.rs1] & registers[instruction.r_type.rs2]; break;
               case SLL:  registers[instruction.rd] = registers[instruction.r_type.rs1] << (registers[instruction.r_type.rs2] & 0x1F); break;
               case SRL:  registers[instruction.rd] = registers[instruction.r_type.rs1] >> (registers[instruction.r_type.rs2] & 0x1F); break;
               case SRA:  registers[instruction.rd] = (int32_t)registers[instruction.r_type.rs1] >> (registers[instruction.r_type.rs2] & 0x1F); break;
               case SLT:  registers[instruction.rd] = ((int32_t)registers[instruction.r_type.rs1] < (int32_t)registers[instruction.r_type.rs2]) ? 1 : 0; break;
               case SLTU: registers[instruction.rd] = (registers[instruction.r_type.rs1] < registers[instruction.r_type.rs2]) ? 1 : 0; break;
            }

            break;
         }

         case OP_IMM: // ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
         {
            int32_t imm = sign_extend(instruction.i_type.imm, 12);
            switch (instruction.i_type.funct3) {
               case ADDI:  registers[instruction.rd] = registers[instruction.i_type.rs1] + imm; break;
               case XORI:  registers[instruction.rd] = registers[instruction.i_type.rs1] ^ imm; break;
               case ORI:   registers[instruction.rd] = registers[instruction.i_type.rs1] | imm; break;
               case ANDI:  registers[instruction.rd] = registers[instruction.i_type.rs1] & imm; break;
               case SLLI:  registers[instruction.rd] = registers[instruction.i_type.rs1] << (imm & 0x1F); break;
               case SRLI_SRAI: {
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
               case SLTI:  registers[instruction.rd] = ((int32_t)registers[instruction.i_type.rs1] < imm) ? 1 : 0; break;
               case SLTIU: registers[instruction.rd] = (registers[instruction.i_type.rs1] < (uint32_t)imm) ? 1 : 0; break;
            }
            break;
         }

         case OP_LUI: // LUI
         {
            registers[instruction.rd] = (uint32_t)instruction.u_type.imm << 12; // LUI
            break;
         }

         case OP_AUIPC: // AUIPC
         {
            registers[instruction.u_type.rd] = pc + (instruction.u_type.imm << 12);
            break;
         }

         case OP_SYSTEM: // ECALL
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
            
            if (registers[a0] == 10) goto end; // exit

            break;
         }

         case OP_BRANCH: // BEQ, BNE, BLT, BGE, BLTU, BGEU
         {
            uint32_t imm = ((instruction.b_type.imm_high & 0x40) << 6)  // bit    6 -> imm[12]
                         | ((instruction.b_type.imm_low & 0x01) << 11)  // bit    0 -> imm[11]
                         | ((instruction.b_type.imm_high & 0x3F) << 5)  // bits 5:0 -> imm[10:5]
                         | ((instruction.b_type.imm_low & 0x1E));       // bits 4:1 -> imm[4:1] 
            int32_t offset = sign_extend(imm, 13);

            bool branch_taken = false;
            switch (instruction.b_type.funct3) {
               case BEQ:  branch_taken = (registers[instruction.b_type.rs1] == registers[instruction.b_type.rs2]); break;
               case BNE:  branch_taken = (registers[instruction.b_type.rs1] != registers[instruction.b_type.rs2]); break;
               case BLT:  branch_taken = ((int32_t)registers[instruction.b_type.rs1] < (int32_t)registers[instruction.b_type.rs2]); break;
               case BGE:  branch_taken = ((int32_t)registers[instruction.b_type.rs1] >= (int32_t)registers[instruction.b_type.rs2]); break;
               case BLTU: branch_taken = (registers[instruction.b_type.rs1] < registers[instruction.b_type.rs2]); break;
               case BGEU: branch_taken = (registers[instruction.b_type.rs1] >= registers[instruction.b_type.rs2]); break;
            }

            if (branch_taken) {
               pc += offset - 4;
            } 

            break;
         }

         case OP_LOAD: // LB, LH, LW, LBU, LHU
         {
            int32_t imm = sign_extend(instruction.i_type.imm, 12);
            uint32_t addr = registers[instruction.i_type.rs1] + imm;
            
            switch (instruction.i_type.funct3) {
               case LB: {
                  int8_t byte;
                  memcpy(&byte, &memory[addr], 1);
                  registers[instruction.rd] = (int32_t)byte;  // Sign-extend
                  break;
               }
               case LH: {
                  int16_t half;
                  memcpy(&half, &memory[addr], 2);
                  registers[instruction.rd] = (int32_t)half;  // Sign-extend
                  break;
               }
               case LW: {
                  memcpy(&registers[instruction.rd], &memory[addr], 4);
                  break;
               }
               case LBU: {
                  uint8_t byte;
                  memcpy(&byte, &memory[addr], 1);
                  registers[instruction.rd] = (uint32_t)byte;  // Zero-extend
                  break;
               }
               case LHU: {
                  uint16_t half;
                  memcpy(&half, &memory[addr], 2);
                  registers[instruction.rd] = (uint32_t)half;  // Zero-extend
                  break;
               }
            }
            break;
         }

         case OP_STORE: // SB, SH, SW
         {
            int32_t imm = sign_extend(instruction.s_type.imm_high << 5 | instruction.s_type.imm_low, 12);
            uint32_t addr = registers[instruction.s_type.rs1] + imm;
            switch (instruction.s_type.funct3) {
               case SB: {
                  memcpy(&memory[addr], (uint8_t*)&registers[instruction.s_type.rs2], 1);
                  break;
               }
               case SH: {
                  memcpy(&memory[addr], (uint16_t*)&registers[instruction.s_type.rs2], 2);
                  break;
               }
               case SW: {
                  memcpy(&memory[addr], (uint32_t*)&registers[instruction.s_type.rs2], 4);
                  break;
               }
            }
            break;
         }

         case OP_JAL: // JAL
         {
            // instruction.j_type.imm is already right shifted by 12
            //    instruction[31]    -> imm[20]
            //    instruction[30:21] -> imm[10:1]
            //    instruction[20]    -> imm[11]
            //    instruction[19:12] -> imm[19:12]
            int32_t imm = sign_extend(
                  ((instruction.j_type.imm & 0x80000) << 1)  // Bit 20
                | ((instruction.j_type.imm & 0x7FE00) >> 8)  // Bit 10:1
                | ((instruction.j_type.imm & 0x00100) << 3)  // Bit 11
                | ((instruction.j_type.imm & 0x000FF) << 12)  // Bit 19:12
                  ,
            21);

            registers[instruction.rd] = pc+4;
            pc += imm-4;

            break;
         }

         case OP_JALR: // JALR
         {
            if (instruction.i_type.funct3 != 0x0) break;

            int32_t imm = sign_extend(instruction.i_type.imm, 12);
            uint32_t target = (registers[instruction.i_type.rs1] + imm) & ~1;

            registers[instruction.rd] = pc + 4;
            pc = target - 4;

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
         DEBUG_PRINT_SIMPLE("0x%08X ", registers[i]);
      }
      DEBUG_PRINT_SIMPLE("\n");

      pc += 4;
      // If pc >= program_size, then undefined behavior according to spec
      //if (pc >= program_size) {
      //   pc = 0;
      //}
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

void execute_r_type(struct rv32_instruction_r_type instruction) {

}
