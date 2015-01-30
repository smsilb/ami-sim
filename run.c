// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

//#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "sim.h"

jmp_buf err_handler;

void raise(struct mips_machine *m, char *msg, ...)
{
  m->R[0] = 0; // insist that $zero stay zero
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);

  unsigned int *inst = mem_ref(m, m->PC, 4);
  if (inst) {
    fprintf(stderr, "MIPS processor choked on instruction 0x%08x at pc 0x%08x\n", *inst, m->PC);
    fprintf(stderr, "Instruction was: ");
    dump_disassembly(stderr, m->PC, *inst);
    longjmp(err_handler, -RUN_FAULT);
  }
  else {
    fprintf(stderr, "MIPS processor choked at illegal pc 0x%08x\n", m->PC);
    longjmp(err_handler, -RUN_FAULT);
  }
  exit(1);
}

int _run(struct mips_machine* m, int count)
{
  int op;
  for (;;) {

    m->R[0] = 0; // insist that $zero stay zero

    if (is_breakpoint(m, m->PC))
      return -RUN_BREAKPOINT;

    int nnPC = m->nPC + 4;

    unsigned int inst = mem_read_word(m, m->PC);

    //need to check that this is an instruction 
    op = m->mem[m->PC].op;

    switch(op) {
      default:
		 raise(m, "FAULT: unknown opcode field of instruction\n");
    }

    m->PC = m->nPC;
    m->nPC = nnPC;

    //ensures only count instructions are executed
    if (count > 0 && --count == 0)
      return -RUN_OK;
  }

}


int run(struct mips_machine* m, int count)
{
  struct timespec tstart, tend;
  int err1 = clock_gettime(CLOCK_MONOTONIC, &tstart);
  int ret = _run(m, count);
  int err2 = clock_gettime(CLOCK_MONOTONIC, &tend);

  if (err1 || err2) {
    perror("clock_gettime");
  }
  if (tend.tv_nsec < tstart.tv_nsec) {
    tend.tv_nsec -= (tstart.tv_nsec - 1000000000);
    tend.tv_sec -= (tstart.tv_sec + 1);
  } else {
    tend.tv_nsec -= (tstart.tv_nsec);
    tend.tv_sec -= (tstart.tv_sec);
  }
  long long nsec = 1000000000 * (long long)tend.tv_sec + (long long)tend.tv_nsec;
  //m->elapsed += nsec;

  return ret;
}

void show_exit_status(struct mips_machine *m)
{
  /*  if (!m->elapsed) {
    printf("MIPS program exits with status %d (approx. %lld instructions at ?? Hz) \n", m->R[REG_A0], m->tsc);
  } else {
    long long hz = m->tsc * 1000000000 / m->elapsed;
    long long khz = hz/1000;
    long long mhz = khz/1000;
    long long ghz = mhz/1000;
    if (ghz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld GHz)\n", m->R[REG_A0], m->tsc, m->elapsed, ghz, mhz);
    else if (mhz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld MHz)\n", m->R[REG_A0], m->tsc, m->elapsed, mhz, khz);
    else if (khz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld KHz)\n", m->R[REG_A0], m->tsc, m->elapsed, khz, hz);
    else 
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld Hz)\n", m->R[REG_A0], m->tsc, m->elapsed, hz);
  }
  */
}

