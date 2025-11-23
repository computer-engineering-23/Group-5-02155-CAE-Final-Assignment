#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instructions.h"
#include "rv32_types.h"

#ifdef DEBUG
   #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s() - " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
   #define DEBUG_PRINT_SIMPLE(fmt, args...) fprintf(stderr, fmt, ##args)
#else
   #define DEBUG_PRINT(fmt, args...) /* Don't do anything on release builds */
   #define DEBUG_PRINT_SIMPLE(fmt, args...) /* Don't do anything on release builds */
#endif /* ifdef  DEBUG */

int main(int argc, char *argv[]) {
   int ret = 0;

   rv32_cpu_t cpu_state = {
      .pc = 0,
      .registers = {0},
      .memory = malloc(memory_size),
      .return_code = 0,
      .running = true,
   };

   if (argc < 2) {
      fprintf(stdout, "Usage: %s <binary_file> [output_file]\n", argv[0]);
      ret = 0;
      goto file_not_specified;
   }

   if (cpu_state.memory == nullptr) {
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
   size_t program_size = fread(cpu_state.memory, 1, memory_size, file);
   fclose(file);
   if (program_size == 0) {
      fprintf(stderr, "File is empty or read error occurred\n");
      ret = -1;
      goto err_file_empty;
   }

   DEBUG_PRINT("Loaded %zu bytes\n", program_size);

   while (cpu_state.running) {
      rv32_instruction_t instruction;
      memcpy(&instruction, &cpu_state.memory[cpu_state.pc], sizeof(instruction));

      DEBUG_PRINT("PC=0x%08X, Instruction=0x%08X, Opcode=0x%02X (0b%07b)\n", cpu_state.pc, instruction.raw, instruction.opcode, instruction.opcode);

      cpu_state.pc += 4;

      switch (instruction.opcode) {
         case OP_REG: // ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
         {
            execute_reg(&cpu_state, instruction.r_type);
            break;
         }

         case OP_IMM: // ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
         {
            execute_imm(&cpu_state, instruction.i_type);
            break;
         }

         case OP_LUI: // LUI
         {
            execute_lui(&cpu_state, instruction.u_type);
            break;
         }

         case OP_AUIPC: // AUIPC
         {
            execute_auipc(&cpu_state, instruction.u_type);
            break;
         }

         case OP_SYSTEM: // ECALL
         {
            execute_system(&cpu_state);
            break;
         }

         case OP_BRANCH: // BEQ, BNE, BLT, BGE, BLTU, BGEU
         {
            execute_branch(&cpu_state, instruction.b_type);
            break;
         }

         case OP_LOAD: // LB, LH, LW, LBU, LHU
         {
            execute_load(&cpu_state, instruction.i_type);
            break;
         }

         case OP_STORE: // SB, SH, SW
         {
            execute_store(&cpu_state, instruction.s_type);
            break;
         }

         case OP_JAL: // JAL
         {
            execute_jal(&cpu_state, instruction.j_type);
            break;
         }

         case OP_JALR: // JALR
         {
            execute_jalr(&cpu_state, instruction.i_type);
            break;
         }
         
         default:
         {
            fprintf(stdout, "Opcode 0x%02X not implemented\n", instruction.opcode);
            break;
         }
      }

      cpu_state.registers[zero] = 0;

      for (int i = 0; i < num_registers; i++) {
         DEBUG_PRINT_SIMPLE("0x%08X ", registers[i]);
      }
      DEBUG_PRINT_SIMPLE("\n");
   }

end:
   char *res_fname = NULL;
   if (argc > 2)
      res_fname = argv[2];

   // Write output to file if file is specified, else write to stdout
   if (res_fname == NULL) {
      for (int i = 0; i < num_registers; i++) {
         fprintf(stdout, "%02X", cpu_state.registers[i]);
      }
      fprintf(stdout, "\n");
   }
   else {
      FILE *res_file = fopen(res_fname, "wb");
      fwrite(cpu_state.registers, 4, num_registers, res_file);
      fclose(res_file);
   }

err_file_not_opened:
err_file_empty:
   free(cpu_state.memory);

file_not_specified:
   return ret | cpu_state.return_code;
}
