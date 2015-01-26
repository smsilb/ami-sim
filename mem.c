// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"

void *_mem_ref(struct mips_machine *m, unsigned int addr, int alignment) {
  //fprintf(stderr, "0x%08x\n", addr);
  if (addr & (alignment - 1))
    return NULL;
  for (int i = 0; i < MAX_SEGMENTS; i++) {
    if (m->mem[i].data &&
	addr >= m->mem[i].addr &&
	addr < m->mem[i].addr + m->mem[i].size)
      return m->mem[i].data + (addr - m->mem[i].addr);
  }
  return NULL;
}

void *mem_ref(struct mips_machine *m, unsigned int addr, int alignment) {
  if (addr & (alignment - 1))
    raise(m,"SEGBUS: Address Error: address 0x%08x is not %d-byte aligned.\n", addr, alignment);
  void *ref = _mem_ref(m, addr, alignment);
  if (!ref)
    raise(m, "SEGFAULT: Address 0x%08x is out of range.\n", addr);
  return ref;
}

char *mem_strdup(struct mips_machine *m, unsigned int addr) {
  int i = 0, size = 100;
  char *buf = malloc(size);
  for (;;) {
    unsigned char c = mem_read_byte(m, addr++);
    buf[i++] = (char)c;
    if (c == '\0')
      return buf;
    if (i == size) {
      size *= 2;
      buf = realloc(buf, size);
    }
  }
}

void mem_read(struct mips_machine *m, unsigned int addr, char *buf, int len) {
  for (int i = 0; i < len; i++) 
    buf[i] = mem_read_byte(m, addr+i);
}

unsigned int mem_read_word(struct mips_machine *m, unsigned int addr) {
  return *(unsigned int*)mem_ref(m, addr, 4);
}

unsigned short mem_read_half(struct mips_machine *m, unsigned int addr) {
  return *(unsigned short*)mem_ref(m, addr, 2);
}

unsigned char mem_read_byte(struct mips_machine *m, unsigned int addr) {
  return *(unsigned char*)mem_ref(m, addr, 1);
}

void mem_write(struct mips_machine *m, unsigned int addr, char *buf, int len) {
    for (int i = 0; i < len; i++) 
      mem_write_byte(m, addr+i, buf[i]);
}

void mem_write_word(struct mips_machine *m, unsigned int addr, unsigned int value) {
  *(unsigned int*)mem_ref(m, addr, 4) = value;
}

void mem_write_half(struct mips_machine *m, unsigned int addr, unsigned short value) {
  *(unsigned short*)mem_ref(m, addr, 2) = value;
}

void mem_write_byte(struct mips_machine *m, unsigned int addr, unsigned char value) {
  *(unsigned char*)mem_ref(m, addr, 1) = value;
}


void dump_segments(struct mips_machine *m) {
  for (int i = 0; i < MAX_SEGMENTS; i++)
    if (m->mem[i].data) 
      printf("segment %d:  addr=0x%08x  size=0x%08x  type=%s\n",
	  i, m->mem[i].addr, m->mem[i].size, m->mem[i].type);
}

// Print the top N elements of the stack
void dump_stack(struct mips_machine *m, int count) {
  unsigned int sp = m->R[REG_SP];
  unsigned int fp = m->R[REG_FP];
  for (unsigned int addr = sp + 4*(count - 1); addr >= sp; addr -= 4) {
    if (addr > m->stack_start) continue;
    char str[5];
    for (int j = 0; j < 4; j++) {
      char c  = mem_read_byte(m, addr + j);
      str[j] = isprint(c) ? c : '.';
    }
    str[4] = '\0';
    if (addr == sp && addr == fp)
      printf("$fp, $sp --> 0x%08x: 0x%08x    %s\n", addr, mem_read_word(m, addr), str);
    else if (addr == sp || addr == fp)
      printf("     $%cp --> 0x%08x: 0x%08x    %s\n", (addr == sp ? 's' : 'f'), addr, mem_read_word(m, addr), str);
    else 
      printf("             0x%08x: 0x%08x    %s\n", addr, mem_read_word(m, addr), str);
  }
}

void dump_mem(struct mips_machine *m, unsigned int addr, int count, int size) {
  // at most 4 words, 8 halfword or 16 bytes
  int printed = 0;
  int row_start = addr;
  for (int i = 0; i < count; i++) {
    if ((i % (16/size)) == 0)
      printf("0x%08x:", addr + i*size);
    switch(size) {
      case 1: printf(" 0x%02x" , mem_read_byte(m, addr + i*size)); break;
      case 2: printf(" 0x%04x" , mem_read_half(m, addr + i*size)); break;
      default: printf(" 0x%08x" , mem_read_word(m, addr + i*size)); break;
    }
    printed += size;
    if (((i+1) % (16/size)) == 0 || (i+1 == count)) {
      if (i >= 16/size) {
	while (((i+1) % (16/size) != 0)) {
	  i++;
	  switch(size) {
	    case 1: printf("     "); break;
	    case 2: printf("       "); break;
	    default: printf("           "); break;
	  }
	}
      }
      char str[17];
      for (int j = 0; j < printed; j++) {
	char c  = mem_read_byte(m, row_start + j);
	str[j] = isprint(c) ? c : '.';
      }
      str[printed] = '\0';
      printf("    %s\n", str);
      row_start += printed;
      printed = 0;
    }
  }
}

struct segment *allocate_segment(struct mips_machine *m, unsigned int addr, unsigned int size, char *type)
{
  for(int i = 0; i < MAX_SEGMENTS; i++) {
    if (m->mem[i].data &&
	addr < m->mem[i].addr + m->mem[i].size &&
	m->mem[i].addr < addr + size) {
      fprintf(stderr, "error allocating memory segment: some segments overlap\n");
      exit(1);
    }
  }

  for(int i = 0; i < MAX_SEGMENTS; i++) {
    if (!m->mem[i].data) {
      m->mem[i].addr = addr;
      m->mem[i].size = size;
      m->mem[i].type = strdup(type);
      m->mem[i].data = calloc(size, 1);
      if (!m->mem[i].data) {
	perror("error allocating memory segment");
	exit(1);
      }
      return &m->mem[i];
    }
  }

  fprintf(stderr, "error allocating memory segment: too many segments\n");
  exit(1);
  return NULL;
}

void free_segments(struct mips_machine *m)
{
  for(int i = 0; i < MAX_SEGMENTS; i++) {
    if (m->mem[i].data) {
      free(m->mem[i].data);
      free(m->mem[i].type);
      m->mem[i].data = NULL;
    }
  }
}

static
unsigned long
hash_djb2(char *str)
{
  unsigned long hash = 5381;
  int c;
  while ((c = (unsigned)*str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

void allocate_stack(struct mips_machine *m)
{
  int r;
  if (m->opt_randkey) {
    r = m->opt_randkey;
    while (r == 0 || r == 1)
      r = m->opt_randkey = rand();
  } else {
    char *key = m->opt_key;
    if (!key)
      key = getenv("NETID");
    if (!key)
      key = getenv("USER");
    if (!key) {
      fprintf(stderr,
	  "Your environment does not contain a $NETID or $USER variable\n"
	  "Please set the NETID variable in your environment, or use the '-r' simulator option.\n");
      exit(1);
    }

    if (key && key[0] == '$') {
      char *s = getenv(key+1);
      if (!s) 
	fprintf(stderr, 
	    "warning: Your environment does not contain a %s variable; using literal string instead.\n",
	    key);
      else
	key = s;
    }

    r = hash_djb2(key);
  }

  // stack top somewhere between 0x70000004 and 0x7FFFFFFc with 8-byte odd alignment
  m->stack_start = 0x70000000 + (0x0FFFFFF8 & (r << 3)) + 4;
  // Special exception: Having 0x0a ('\r') or 0x0d ('\n') appear
  // in the stack address makes the stack smashing homework very hard.
  // We check for that here and just change those bytes to something else.
  // We take off 100 bytes to account for the first few stack frames.
  if ((((m->stack_start - 100) >> 16) & 0xff) == '\r') m->stack_start ^= 0x00800000;
  if ((((m->stack_start - 100) >> 16) & 0xff) == '\n') m->stack_start ^= 0x00800000;
  if ((((m->stack_start - 100) >> 8) & 0xff) == '\r') m->stack_start ^= 0x00008000;
  if ((((m->stack_start - 100) >> 8) & 0xff) == '\n') m->stack_start ^= 0x00008000;

  allocate_segment(m, m->stack_start - STACK_SIZE + 4, STACK_SIZE + 4096, ".stack");
  m->R[REG_SP] = m->stack_start;
}

void push_arguments(struct mips_machine *m)
{
  unsigned int sp = m->stack_start;
  int ac = m->opt_ac;
  char **av = m->opt_av;
  // push array of ac+1 pointers 
  sp -= (ac+1)*4;
  if (sp & 7) sp -= 4; // realign to 8-byte boundary
  unsigned int av_addr = sp;
  // push the strings
  for (int i = 0; i < ac; i++) {
    int n = strlen(av[i]) + 1;
    sp -= 8 * ((n + 7) / 8); // round to multiple of 8
    mem_write(m, sp, av[i], n);
    mem_write_word(m, av_addr+4*i, sp);
  }
  mem_write_word(m, av_addr+4*ac, 0); // last ac[] entry is NULL

  sp -= 4*4; // push four empty slots

  m->R[REG_SP] = sp;
  m->R[REG_A0] = ac;
  m->R[REG_A1] = av_addr;
}


struct mips_machine *create_mips_machine()
{
  struct mips_machine *m = calloc(sizeof(struct mips_machine), 1);
  m->opt_input = stdin;
  m->opt_output = stdout;
  return m;
}
