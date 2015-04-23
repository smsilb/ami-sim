// Copyright (c) 2015, Sam Silberstein.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: smsilb14@g.holycross.edu

#ifndef SIM_H
#define SIM_H

#include <stdio.h>
#include <setjmp.h>

#define TRUE 1
#define FALSE 0

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
  MOVE, IDM, LOAD, STORE, EQ, NEQ, LT, LTE, AND, OR, 
  NOT, ADD, SUB, MULT, DIV, NEG
};

/*
  Types of address values
 */
enum {
  DISP, REG
};
/*
  Types of arguments
 */
enum {
  NUMBER, REGISTER, ADDRESS
};

/*
  Types of stack_entrys
 */
enum {
  INSTRUCTION, DATA
};

struct address{
  int value;
  unsigned int type;
};

struct argument {
  int number;
  unsigned int reg, addc, type;
  struct address add[3];
};

struct stack_entry {
  char *instruction;
  int data;
  unsigned int op, data_type, argc;
  struct argument arguments[3];
};

#define MAX_SEGMENTS 16
#define MAX_REGISTERS 100
#define STACK_SIZE 256


struct breakpoint {
  int id;
  int enabled;
  unsigned int addr;
  struct breakpoint *next;
};

struct ami_machine {
    /* debug options */
    int opt_printstack;//for 'print' command with stack
    int opt_dumpreg;//for 'print' command with registers
    int opt_graphical;//to select graphical or text mode
    char *filename;//holds the name of the input file
    int opt_ac;//command line argument count
    char **opt_av;//command line arguments
    struct breakpoint *breakpoints;//list of breakpoints

    /* gui management */
    char *shm;//pointer to shared memory
    int console_io_status;//holds status code for GUI console
    int console_io_value;//holds value to pass between sim and
                         //GUI console

    /* run state */
    int halted;//halts the simulator after executing a 'halt' command

    /* memory state */
    struct stack_entry mem[STACK_SIZE];//virtual memory for 
                                       //instructions & data
    unsigned int slots_used;//# of mem slots that are instructions

    /* CPU registers */
    int R[MAX_REGISTERS];//virtual registers
    unsigned int PC, nPC;//program counter, next program counter
    int reg_count;//count of registers (for printing)
};


struct ami_machine *create_ami_machine(void);
void allocate_stack(struct ami_machine *m);
void push_arguments(struct ami_machine *m);
void free_segments(struct ami_machine *m);
struct stack_entry *allocate_segment(struct ami_machine *m, unsigned int addr, unsigned int size, char *type);

void dump_segments(struct ami_machine *m);
void dump_registers(struct ami_machine *m);
void dump_stack(struct ami_machine *m, int start);
void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst);
void dump_mem(struct ami_machine *m, unsigned int addr, int count, int size);

int arg_get_value(struct ami_machine *m, struct argument arg);
int add_get_value(struct ami_machine *m, struct argument add);
int mem_get_addr(struct ami_machine *m, struct argument arg);
int mem_read(struct ami_machine *m, unsigned int addr);
void mem_write(struct ami_machine *m, unsigned int addr, int value);
char *read_stack_entry(struct ami_machine *m, int addr);

char *readfile(char *filename);

struct stack_entry disasm_instr(struct ami_machine *m, char *instr);
char *read_argument(struct stack_entry *ret, char *token, char *stop_words[], int words);
void init_stop_words(char *stop_words[]);

enum { RUN_OK=0, RUN_BREAK=1, RUN_BREAKPOINT=2, RUN_FAULT=3, RUN_EXIT=4, RUN_HALTED=5 };
int run(struct ami_machine* m, int count);
void show_exit_status(struct ami_machine *m);
void update_gui(struct ami_machine *m);
void interactive_debug(struct ami_machine* m);
int is_breakpoint(struct ami_machine *m, unsigned int addr);
int dosyscall(struct ami_machine *m);
void raise(struct ami_machine *m, char *msg) __attribute__ ((noreturn));
extern jmp_buf err_handler;



#endif // SIM_H
