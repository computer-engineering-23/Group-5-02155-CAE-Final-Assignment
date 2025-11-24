#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "rv32_types.h"
#include "instructions.h"

// ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU
void execute_reg(rv32_cpu_t *cpu_state, struct rv32_instruction_r_type instruction) {
    uint16_t funct = (instruction.funct7 << 3) | instruction.funct3;
    switch (funct) {
        case ADD:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] + cpu_state->registers[instruction.rs2]; break;
        case SUB:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] - cpu_state->registers[instruction.rs2]; break;
        case XOR:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] ^ cpu_state->registers[instruction.rs2]; break;
        case OR:   cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] | cpu_state->registers[instruction.rs2]; break;
        case AND:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] & cpu_state->registers[instruction.rs2]; break;
        case SLL:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] << (cpu_state->registers[instruction.rs2] & 0x1F); break;
        case SRL:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] >> (cpu_state->registers[instruction.rs2] & 0x1F); break;
        case SRA:  cpu_state->registers[instruction.rd] = (int32_t)cpu_state->registers[instruction.rs1] >> (cpu_state->registers[instruction.rs2] & 0x1F); break;
        case SLT:  cpu_state->registers[instruction.rd] = ((int32_t)cpu_state->registers[instruction.rs1] < (int32_t)cpu_state->registers[instruction.rs2]) ? 1 : 0; break;
        case SLTU: cpu_state->registers[instruction.rd] = (cpu_state->registers[instruction.rs1] < cpu_state->registers[instruction.rs2]) ? 1 : 0; break;
    }
}

// ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU
void execute_imm(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction) {
    int32_t imm = sign_extend(instruction.imm, 12);
    switch (instruction.funct3) {
        case ADDI:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] + imm; break;
        case XORI:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] ^ imm; break;
        case ORI:   cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] | imm; break;
        case ANDI:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] & imm; break;
        case SLLI:  cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] << (imm & 0x1F); break;
        case SRLI_SRAI: {
            uint8_t shift_amount = instruction.imm & 0x1F;  // How many positions to shift
            uint8_t mode = get_bits(instruction.imm, 11, 5);
            if (mode == 0x00) { // SRLI
                cpu_state->registers[instruction.rd] = cpu_state->registers[instruction.rs1] >> shift_amount;
            }
            else { // SRLAI
                cpu_state->registers[instruction.rd] = (int32_t)cpu_state->registers[instruction.rs1] >> shift_amount;
            }
            break;
        }
        case SLTI:  cpu_state->registers[instruction.rd] = ((int32_t)cpu_state->registers[instruction.rs1] < imm) ? 1 : 0; break;
        case SLTIU: cpu_state->registers[instruction.rd] = (cpu_state->registers[instruction.rs1] < (uint32_t)imm) ? 1 : 0; break;
    }
}

// BEQ, BNE, BLT, BGE, BLTU, BGEU
void execute_branch(rv32_cpu_t *cpu_state, struct rv32_instruction_b_type instruction) {
    uint32_t imm = ((instruction.imm_high & 0x40) << 6)  // bit    6 -> imm[12]
                 | ((instruction.imm_low & 0x01) << 11)  // bit    0 -> imm[11]
                 | ((instruction.imm_high & 0x3F) << 5)  // bits 5:0 -> imm[10:5]
                 | ((instruction.imm_low & 0x1E));       // bits 4:1 -> imm[4:1] 
    int32_t offset = sign_extend(imm, 13);

    bool branch_taken = false;
    switch (instruction.funct3) {
        case BEQ:  branch_taken = (cpu_state->registers[instruction.rs1] == cpu_state->registers[instruction.rs2]); break;
        case BNE:  branch_taken = (cpu_state->registers[instruction.rs1] != cpu_state->registers[instruction.rs2]); break;
        case BLT:  branch_taken = ((int32_t)cpu_state->registers[instruction.rs1] < (int32_t)cpu_state->registers[instruction.rs2]); break;
        case BGE:  branch_taken = ((int32_t)cpu_state->registers[instruction.rs1] >= (int32_t)cpu_state->registers[instruction.rs2]); break;
        case BLTU: branch_taken = (cpu_state->registers[instruction.rs1] < cpu_state->registers[instruction.rs2]); break;
        case BGEU: branch_taken = (cpu_state->registers[instruction.rs1] >= cpu_state->registers[instruction.rs2]); break;
    }

    if (branch_taken) {
        cpu_state->pc += offset-4;
    }
}

// LB, LH, LW, LBU, LHU
void execute_load(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction) {
    int32_t imm = sign_extend(instruction.imm, 12);
    uint32_t addr = cpu_state->registers[instruction.rs1] + imm;
    
    switch (instruction.funct3) {
        case LB: {
            int8_t byte;
            memcpy(&byte, &cpu_state->memory[addr], 1);
            cpu_state->registers[instruction.rd] = (int32_t)byte;  // Sign-extend
            break;
        }
        case LH: {
            int16_t half;
            memcpy(&half, &cpu_state->memory[addr], 2);
            cpu_state->registers[instruction.rd] = (int32_t)half;  // Sign-extend
            break;
        }
        case LW: {
            memcpy(&cpu_state->registers[instruction.rd], &cpu_state->memory[addr], 4);
            break;
        }
        case LBU: {
            uint8_t byte;
            memcpy(&byte, &cpu_state->memory[addr], 1);
            cpu_state->registers[instruction.rd] = (uint32_t)byte;  // Zero-extend
            break;
        }
        case LHU: {
            uint16_t half;
            memcpy(&half, &cpu_state->memory[addr], 2);
            cpu_state->registers[instruction.rd] = (uint32_t)half;  // Zero-extend
            break;
        }
    }
}

// SB, SH, SW
void execute_store(rv32_cpu_t *cpu_state, struct rv32_instruction_s_type instruction) {
    int32_t imm = sign_extend(instruction.imm_high << 5 | instruction.imm_low, 12);
    uint32_t addr = cpu_state->registers[instruction.rs1] + imm;
    switch (instruction.funct3) {
        case SB: {
            memcpy(&cpu_state->memory[addr], (uint8_t*)&cpu_state->registers[instruction.rs2], 1);
            break;
        }
        case SH: {
            memcpy(&cpu_state->memory[addr], (uint16_t*)&cpu_state->registers[instruction.rs2], 2);
            break;
        }
        case SW: {
            memcpy(&cpu_state->memory[addr], (uint32_t*)&cpu_state->registers[instruction.rs2], 4);
            break;
        }
    }
}

// JAL
void execute_jal(rv32_cpu_t *cpu_state, struct rv32_instruction_j_type instruction) {
    // instruction.j_type.imm is already right shifted by 12
    //    instruction[31]    -> imm[20]
    //    instruction[30:21] -> imm[10:1]
    //    instruction[20]    -> imm[11]
    //    instruction[19:12] -> imm[19:12]
    int32_t imm = sign_extend(
            ((instruction.imm & 0x80000) << 1)  // Bit 20
        | ((instruction.imm & 0x7FE00) >> 8)  // Bit 10:1
        | ((instruction.imm & 0x00100) << 3)  // Bit 11
        | ((instruction.imm & 0x000FF) << 12)  // Bit 19:12
            ,
    21);

    cpu_state->registers[instruction.rd] = cpu_state->pc;
    cpu_state->pc += imm-4;
}

// JALR
void execute_jalr(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction) {
    if (instruction.funct3 != 0x0) return;

    int32_t imm = sign_extend(instruction.imm, 12);
    uint32_t target = (cpu_state->registers[instruction.rs1] + imm) & ~1;

    cpu_state->registers[instruction.rd] = cpu_state->pc;
    cpu_state->pc = target;
}

// LUI
void execute_lui(rv32_cpu_t *cpu_state, struct rv32_instruction_u_type instruction) { 
    cpu_state->registers[instruction.rd] = (uint32_t)instruction.imm << 12;
}

// AUIPC
void execute_auipc(rv32_cpu_t *cpu_state, struct rv32_instruction_u_type instruction) {
    cpu_state->registers[instruction.rd] = cpu_state->pc-4 + (instruction.imm << 12);
}

// ECALL
void execute_system(rv32_cpu_t *cpu_state) {
    uint32_t *registers = cpu_state->registers;

    switch (registers[a7]) {
        case 1: fprintf(stdout, "%d", (int32_t)registers[a0]); break; // print_int
        case 2: fprintf(stdout, "%f", (float)registers[a0]); break; // print_float
        case 4: fprintf(stdout, "%s", (char *)&cpu_state->memory[registers[a0]]); break; // print_string
        case 10: cpu_state->running = false; break; // exit
        case 11: fprintf(stdout, "%c", (char)registers[a0]); break; // print_char
        case 34: fprintf(stdout, "0x%02X", registers[a0]); break; // print_hex
        case 35: fprintf(stdout, "0b%08b", registers[a0]); break; // print_bin
        case 36: fprintf(stdout, "%u", registers[a0]); break; // print_unsigned
        case 93: cpu_state->return_code = cpu_state->registers[a0]; cpu_state->running = false; break; // exit
    }
    
    if (registers[a0] == 10) cpu_state->running = false; // exit
}
