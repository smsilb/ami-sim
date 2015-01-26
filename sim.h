// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#ifndef SIM_H
#define SIM_H

#include <stdio.h>
#include <setjmp.h>

struct segment {
  unsigned int addr, size;
  char *type;
  void *data;
};
#define MAX_SEGMENTS 16
#define STACK_SIZE (1024*1024*4)

enum {
  REG_0,  REG_AT, REG_V0, REG_V1, REG_A0, REG_A1, REG_A2, REG_A3,
  REG_T0, REG_T1, REG_T2, REG_T3, REG_T4, REG_T5, REG_T6, REG_T7,
  REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7,
  REG_T8, REG_T9, REG_K0, REG_K1, REG_GP, REG_SP, REG_FP, REG_RA
};

struct breakpoint {
  int id;
  int enabled;
  unsigned int addr;
  struct breakpoint *next;
};

struct mips_machine {
  /* debug options */
  int opt_disassemble;
  int opt_printstack;
  int opt_dumpreg;
  char *elf_filename;
  char *opt_key; // stack randomization key
  int opt_randkey; // use rand() instead of key
  int opt_ac;
  char **opt_av;
  char *opt_dir;
  FILE *opt_input, *opt_output;
  struct breakpoint *breakpoints;
  unsigned long long tsc; // cycles executed
  unsigned long long elapsed; // elapsed time in nanoseconds

  /* memory state */
  struct segment mem[MAX_SEGMENTS];
  unsigned int stack_start;
  unsigned int entry_point;

  /* CPU registers */
  unsigned int R[32], LO, HI, PC, nPC;
  /* Coprocessor register */
  unsigned int CpCond[4], CCR[4][32], CPR[4][32];
  /* FPU registers */
  union {
    float FGR[16];
    double FPR[8];
    int FWR[16];
  };
};


struct mips_machine *create_mips_machine(void);
void allocate_stack(struct mips_machine *m);
void push_arguments(struct mips_machine *m);
void free_segments(struct mips_machine *m);
struct segment *allocate_segment(struct mips_machine *m, unsigned int addr, unsigned int size, char *type);

void dump_segments(struct mips_machine *m);
void dump_registers(struct mips_machine *m);
void dump_stack(struct mips_machine *m, int count);
void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst);
void dump_mem(struct mips_machine *m, unsigned int addr, int count, int size);

void *_mem_ref(struct mips_machine *m, unsigned int addr, int alignment);
void *mem_ref(struct mips_machine *m, unsigned int addr, int alignment);
char *mem_strdup(struct mips_machine *m, unsigned int addr);
void mem_read(struct mips_machine *m, unsigned int addr, char *buf, int len);
unsigned int mem_read_word(struct mips_machine *m, unsigned int addr);
unsigned short mem_read_half(struct mips_machine *m, unsigned int addr);
unsigned char mem_read_byte(struct mips_machine *m, unsigned int addr);
void mem_write(struct mips_machine *m, unsigned int addr, char *buf, int len);
void mem_write_word(struct mips_machine *m, unsigned int addr, unsigned int value);
void mem_write_half(struct mips_machine *m, unsigned int addr, unsigned short value);
void mem_write_byte(struct mips_machine *m, unsigned int addr, unsigned char value);

void readelf(struct mips_machine *m, char *filename);

enum { RUN_OK=0, RUN_BREAK=1, RUN_BREAKPOINT=2, RUN_FAULT=3, RUN_EXIT=4 };
int run(struct mips_machine* m, int count);
void show_exit_status(struct mips_machine *m);
void interactive_debug(struct mips_machine* m);
int is_breakpoint(struct mips_machine *m, unsigned int addr);
int dosyscall(struct mips_machine *m);
void raise(struct mips_machine *m, char *msg, ...) __attribute__ ((noreturn));
extern jmp_buf err_handler;


#define SIGNEX(X) (((X) & 0x8000) ? ((X) | 0xffff0000) : (X))
#define NEG(X) (((X) & 0x80000000) != 0)
#define FpCond (m->CPR[1][31])

enum { /* MIPS R2000/3000 Instructions */
  SPECIAL, REGIMM, J, JAL, BEQ, BNE, BLEZ, BGTZ, ADDI, ADDIU, SLTI,
  SLTIU, ANDI, ORI, XORI, LUI, COP0, COP1, COP2, COP3,
  LB=32, LH, LWL, LW, LBU, LHU, LWR,
  SB=40, SH, SWL, SW, SWR=46,
  LWC0=48, LWC1, LWC2, LWC3,
  SWC0=56, SWC1, SWC2, SWC3
};

enum { /* SPECIAL function */
  SLL, SRL=2, SRA, SLLV, SRLV=6, SRAV,
  JR, JALR, SYSCALL=12, BREAK,
  MFHI=16, MTHI, MFLO, MTLO,
  MULT=24, MULTU, DIV, DIVU,
  ADD=32, ADDU, SUB, SUBU, AND, OR, XOR, NOR,
  SLT=42, SLTU
};

enum { BLTZ, BGEZ, BLTZAL=16, BGEZAL}; /* REGIMM rt */
enum { MF=0, CF=2, MT=4, CT=6, BC=8, CO=16, FF_S=16, FF_D, FF_W=20 }; /* COPz rs */
enum { S = 0, D, DUMMY1, DUMMY2, W }; /* COPz res */
enum { BCF, BCT }; /* COPz rt */

enum { /* MIPS R2010/3010 Floating Point Unit */
  FADD, FSUB, FMUL, FDIV, FABS=5, FMOV, FNEG,
  FCVTS=32, FCVTD, FCVTW=36,
  C_F=48, CUN, CEQ, CUEQ, COLT, CULT, COLE, CULE,
  CSF, CNGLE, CSEQ, CNGL,  CLT, CNGE,  CLE, CNGT
};

enum {COND_UN=0x1, COND_EQ=0x2, COND_LT=0x4, COND_IN=0x8}; /* FP cond */

#endif // SIM_H
