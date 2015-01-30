// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#ifndef SIM_H
#define SIM_H

#include <stdio.h>
#include <setjmp.h>

/*
  Instruction IDs for:
  Halt, write, read boolean, read integer, jump if condition is true,
  jump if condition not true, jump unconditional, move register to register,
  move address to address, immediate data move, immediate data move w/ negative 
  number, load, store, register comparisons (equal to, not equal to, less than,
  less than or equal to), numerical arithmetic (and, or, not, addition, 
  subtraction, multiplication, division, negation)
 */
enum {
  HALT, WRITE, READB, READI, JUMPIF, JUMPNIF, JUMP,
  MOVRTR, MOVATA, IDM, IDMN, LOAD, STORE,
  EQ, NEQ, LT, LTE, AND, OR, NOT, ADD, SUB, MULT, DIV, NEG
};

/*
  Types of arguments
 */
enum {
  NUMBER, REGISTER, ADDRESS
};

/*
  Types of stackEntrys
 */
enum {
  INSTRUCTION, DATA
};

struct argument {
  int number;
  unsigned int register, adBase, adDisp, type;
};

struct stackEntry {
  char *instruction;
  int data;
  unsigned int op, type;
  struct argument arg1, arg2, arg3;
};

#define MAX_SEGMENTS 16
#define STACK_SIZE 1024

struct breakpoint {
  int id;
  int enabled;
  unsigned int addr;
  struct breakpoint *next;
};

struct mips_machine {
  /* debug options */
  int opt_printstack;
  int opt_dumpreg;
  char *elf_filename;
  int opt_ac;
  char **opt_av;
  char *opt_dir;
  FILE *opt_input, *opt_output;
  struct breakpoint *breakpoints;

  /* memory state */
  struct stackEntry mem[STACK_SIZE];

  /* CPU registers */
  unsigned int *R = (int*) malloc(sizeof(int) * 10), PC, nPC;
  int reg_count = 10;
};


struct mips_machine *create_mips_machine(void);
void allocate_stack(struct mips_machine *m);
void push_arguments(struct mips_machine *m);
void free_segments(struct mips_machine *m);
struct stackEntry *allocate_segment(struct mips_machine *m, unsigned int addr, unsigned int size, char *type);

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



#endif // SIM_H
