#pragma once
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "rv32_types.h"

void execute_reg(rv32_cpu_t *cpu_state, struct rv32_instruction_r_type instruction);
void execute_imm(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction);
void execute_branch(rv32_cpu_t *cpu_state, struct rv32_instruction_b_type instruction);
void execute_load(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction);
void execute_store(rv32_cpu_t *cpu_state, struct rv32_instruction_s_type instruction);
void execute_jal(rv32_cpu_t *cpu_state, struct rv32_instruction_j_type instruction);
void execute_jalr(rv32_cpu_t *cpu_state, struct rv32_instruction_i_type instruction);
void execute_lui(rv32_cpu_t *cpu_state, struct rv32_instruction_u_type instruction);
void execute_auipc(rv32_cpu_t *cpu_state, struct rv32_instruction_u_type);

void execute_system(rv32_cpu_t *cpu_state);
#endif