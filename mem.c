// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"


void *mem_ref(struct ami_machine *m, unsigned int addr, int alignment) {
  printf("Referencing memory at address: %i\n", addr);
}

char *mem_strdup(struct ami_machine *m, unsigned int addr) {
  printf("Duplicating memory at address: %i\n", addr);
}

void mem_read(struct ami_machine *m, unsigned int addr, char *buf, int len) {
  printf("Reading %i bytes from %i\n", len, addr);
}

unsigned int mem_read_word(struct ami_machine *m, unsigned int addr) {
  return *(unsigned int*)mem_ref(m, addr, 4);
}

unsigned short mem_read_half(struct ami_machine *m, unsigned int addr) {
  return *(unsigned short*)mem_ref(m, addr, 2);
}

unsigned char mem_read_byte(struct ami_machine *m, unsigned int addr) {
  return *(unsigned char*)mem_ref(m, addr, 1);
}

void mem_write(struct ami_machine *m, unsigned int addr, char *buf, int len) {
  printf("Writing %i bytes to %i\n", len, addr);
}

void mem_write_word(struct ami_machine *m, unsigned int addr, unsigned int value) {
  *(unsigned int*)mem_ref(m, addr, 4) = value;
}

void mem_write_half(struct ami_machine *m, unsigned int addr, unsigned short value) {
  *(unsigned short*)mem_ref(m, addr, 2) = value;
}

void mem_write_byte(struct ami_machine *m, unsigned int addr, unsigned char value) {
  *(unsigned char*)mem_ref(m, addr, 1) = value;
}


void dump_segments(struct ami_machine *m) {
  printf("dumping segments\n");
}

void dump_stack(struct ami_machine *m, int count) {
  printf("Stack entries:\n");

  int i;
  for (i = 0; i < m->slots_used; i++) {
    if (m->mem[i].data_type == INSTRUCTION) {
      printf("%s\n", m->mem[i].instruction);
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
  printf("freeing segments\n");
}

void allocate_stack(struct ami_machine *m)
{
  char *line;
  int line_count = 0;

  char *file = readfile(m->filename);

  line = strtok(file, "\n");

  while (line != NULL) {
    m->mem[line_count].instruction = (char*) malloc(strlen(line) + 1);
    strcpy(m->mem[line_count].instruction, line);

    line = strtok(NULL, "\n");
    line_count++;
  }

  int i;
  for (i = 0; i < line_count; i++) {
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
