// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"

int arg_get_value(struct ami_machine *m, struct argument arg) {
  if (arg.type == REGISTER) {
    return m->R[arg.reg];
  } else if (arg.type == ADDRESS) {
    return m->mem[mem_get_addr(m, arg)].data;
  } else {
    raise(m, "Non register/address argument supplied");
  }
}

int add_get_value(struct ami_machine *m, struct argument add) {
  if (add.type == REGISTER) {
    return m->R[add.reg];
  } else {
    return mem_get_addr(m, add);
  }
}

int mem_read(struct ami_machine *m, unsigned int addr) {
  if (m->mem[addr].data_type == DATA) {
    return m->mem[addr].data;
  } else {
    raise(m, "Inappropriate memory access");
  }
}

int mem_get_addr(struct ami_machine *m, struct argument arg) {
  if (arg.type == NUMBER) {
    return arg.number;
  } else if (arg.type == REGISTER) {
    return arg.reg;
  } else {
    int sum = 0, i;
    
    for (i = 0; i < arg.addc; i++) {
      if (arg.add[i].type == REG) {
	sum += m->R[arg.add[i].value];
      } else {
	sum += arg.add[i].value;
      }
    }
    return sum;
  }
}

void mem_write(struct ami_machine *m, unsigned int addr, int value) {
  if (m->mem[addr].data_type == INSTRUCTION) {
    raise(m, "Attempted to overwrite instruction");
  } else {
    m->mem[addr].data_type == DATA;
    m->mem[addr].data = value;
  }
}

void dump_segments(struct ami_machine *m) {
  printf("dumping segments\n");
}

void dump_stack(struct ami_machine *m, int start) {
  printf("Stack entries:\n");

  int i, end = (start + 25 > STACK_SIZE) ? STACK_SIZE : start + 25;

  for (i = start; i < end; i++) {
    if (m->mem[i].data_type == INSTRUCTION) {
      printf("%s\n", m->mem[i].instruction);
    } else {
      printf("%i: %i\n", i, m->mem[i].data);
    }
  }
}

void dump_mem(struct ami_machine *m, unsigned int addr, int count, int size) {
  printf("dumping memory\n");
}

struct stack_entry *allocate_segment(struct ami_machine *m, unsigned int addr, unsigned int size, char *type)
{
  printf("Allocating segment\n");
  return NULL;
}

void free_segments(struct ami_machine *m)
{
  int i;
  for (i = 0; i < STACK_SIZE; i++) {
    m->mem[i].data = 0;
  }
}

void allocate_stack(struct ami_machine *m)
{
  char *line;
  int line_count = 0;

  char *file = readfile(m->filename);
  
  int i;
  for (i = 0; i < STACK_SIZE; i++) {
    m->mem[i].data_type = DATA;
  }

  line = strtok(file, "\n");

  while (line != NULL) {
    m->mem[line_count].instruction = (char*) malloc(strlen(line) + 1);
    strcpy(m->mem[line_count].instruction, line);

    line = strtok(NULL, "\n");
    line_count++;
  }

  for (i = 0; i < line_count; i++) {
    printf("Disassembling line %i\n", i);
    m->mem[i] = disasm_instr(m, m->mem[i].instruction);
  }

  m->slots_used = line_count;
}

void push_arguments(struct ami_machine *m)
{
  printf("Pushing arguments\n");
}


struct ami_machine *create_ami_machine()
{
  struct ami_machine *m = calloc(sizeof(struct ami_machine), 1);
  return m;
}
