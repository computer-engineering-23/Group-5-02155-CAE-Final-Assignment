#pragma once
#ifndef RV32_TYPES_H
#define RV32_TYPES_H

#include <stdint.h>

typedef struct {
    uint32_t pc;
    uint32_t registers[32];
    uint8_t *memory;
    int return_code;
    bool running;
} rv32_cpu_t;

enum RV32_REGISTERS {
   x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,
   x8,  x9,  x10, x11, x12, x13, x14, x15,
   x16, x17, x18, x19, x20, x21, x22, x23,
   x24, x25, x26, x27, x28, x29, x30, x31,

   // ABI names
   zero = x0, ra = x1,  sp = x2,  gp = x3,
   tp = x4,   t0 = x5,  t1 = x6,  t2 = x7,
   s0 = x8,   fp = x8,  s1 = x9, 
   a0 = x10,  a1 = x11, a2 = x12, a3 = x13,
   a4 = x14,  a5 = x15, a6 = x16, a7 = x17,
   s2 = x18,  s3 = x19, s4 = x20, s5 = x21,
   s6 = x22,  s7 = x23, s8 = x24, s9 = x25,
   s10 = x26, s11 = x27,
   t3 = x28,  t4 = x29, t5 = x30, t6 = x31,
};

typedef union {
   uint32_t raw; // Raw 32-bit instruction

   // These fields are universal
   struct {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t __unused : 20;
   };

   struct rv32_instruction_r_type {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t rs2 : 5;
      uint32_t funct7 : 7;
   } r_type;

   struct rv32_instruction_i_type {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t imm : 12;
   } i_type;

   struct rv32_instruction_s_type {
      uint32_t opcode : 7;
      uint32_t imm_low : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t rs2 : 5;
      uint32_t imm_high : 7;
   } s_type;

   struct rv32_instruction_b_type {
      uint32_t opcode : 7;
      uint32_t imm_low : 5;
      uint32_t funct3 : 3;
      uint32_t rs1 : 5;
      uint32_t rs2 : 5;
      uint32_t imm_high : 7;
   } b_type;

   struct rv32_instruction_u_type {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t imm : 20;
   } u_type;

   struct rv32_instruction_j_type {
      uint32_t opcode : 7;
      uint32_t rd : 5;
      uint32_t imm : 20;
   } j_type;

} rv32_instruction_t;

typedef enum RV32OPCODES {
   OP_LOAD   = 0b0000011, // 0x03 - LB, LH, LW, LBU, LHU
   OP_IMM    = 0b0010011, // 0x13 - ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
   OP_AUIPC  = 0b0010111, // 0x17 - AUIPC
   OP_STORE  = 0b0100011, // 0x23 - SB, SH, SW
   OP_REG    = 0b0110011, // 0x33 - ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
   OP_LUI    = 0b0110111, // 0x37 - LUI
   OP_BRANCH = 0b1100011, // 0x63 - BEQ, BNE, BLT, BGE, BLTU, BGEU
   OP_JALR   = 0b1100111, // 0x67 - JALR
   OP_JAL    = 0b1101111, // 0x6F - JAL
   OP_SYSTEM = 0b1110011, // 0x73 - ECALL, EBREAK
} rv32opcodes_t;

// R-type instructions (OP_REG) - Combined funct7 and funct3
enum rv32_r_type_instructions {
   ADD  = (0x00 << 3) | 0x0,
   SUB  = (0x20 << 3) | 0x0,
   XOR  = (0x00 << 3) | 0x4,
   OR   = (0x00 << 3) | 0x6,
   AND  = (0x00 << 3) | 0x7,
   SLL  = (0x00 << 3) | 0x1,
   SRL  = (0x00 << 3) | 0x5,
   SRA  = (0x20 << 3) | 0x5,
   SLT  = (0x00 << 3) | 0x2,
   SLTU = (0x00 << 3) | 0x3,
};

// I-type immediate instructions (OP_IMM) - funct3 values
enum rv32_i_type_instructions {
   ADDI  = 0x0,
   SLLI  = 0x1,
   SLTI  = 0x2,
   SLTIU = 0x3,
   XORI  = 0x4,
   SRLI_SRAI = 0x5, // Distinguished by bit 30 of immediate
   ORI   = 0x6,
   ANDI  = 0x7,
};

// Load instructions (OP_LOAD) - funct3 values
enum rv32_load_instructions {
   LB  = 0x0,
   LH  = 0x1,
   LW  = 0x2,
   LBU = 0x4,
   LHU = 0x5,
};

// Store instructions (OP_STORE) - funct3 values
enum rv32_store_instructions {
   SB = 0x0,
   SH = 0x1,
   SW = 0x2,
};

// Branch instructions (OP_BRANCH) - funct3 values
enum rv32_branch_instructions {
   BEQ  = 0x0,
   BNE  = 0x1,
   BLT  = 0x4,
   BGE  = 0x5,
   BLTU = 0x6,
   BGEU = 0x7,
};

#endif