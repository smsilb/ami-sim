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

void raise(struct ami_machine *m, char *msg)
{
  char *inst = m->mem[m->PC].instruction;
  if (inst) {
    printf("AMI processor choked on instruction %s with message: %s\n", inst, msg);
  } else {
    printf("AMI processor choked at illegal pc %d with message: %s\n", m->PC, msg);
  }
  exit(1);
}

int _run(struct ami_machine* m, int count)
{
  int op, addr;
  struct stack_entry entry;
  for (;;) {
    //    if (m->halted)
    //return -RUN_HALTED;

    if (is_breakpoint(m, m->PC)) {
      return -RUN_BREAKPOINT;
    }

    m->nPC = m->PC + 1;

    op = m->mem[m->PC].op;
    entry = m->mem[m->PC];
    printf("Running line %s\n", entry.instruction);

    switch(op) {
    case HALT:
      printf("HALT\n");
      m->halted = 1;
      break;
    case WRITE:
      printf("WRITE\n");
      break;
    case READB:
      printf("READB\n");
      break;
    case READI:
      printf("READI\n");
      break;
    case JUMP:
      addr = mem_get_addr(m, entry.arguments[0]);
      printf("JUMP to %i\n", addr);
      break;
    case JUMPIF:
      addr = mem_get_addr(m, entry.arguments[0]);
      if (arg_get_value(m, entry.arguments[1])) {
	printf("JUMPIF to %i, COND TRUE\n", addr);
      } else {
	printf("JUMPIF to %i, COND FALSE\n", addr);
      }
      break;
    case JUMPNIF:
      addr = mem_get_addr(m, entry.arguments[0]);
      if (arg_get_value(m, entry.arguments[1]) == 0) {
	printf("JUMPNIF to %i, COND TRUE\n", addr);
      } else {
	printf("JUMPNIF to %i, COND FALSE\n", addr);
      }
      break;
    case MOVE:
      if (entry.arguments[0].type == REGISTER) {
	m->R[entry.arguments[0].reg] = arg_get_value(m, entry.arguments[1]);
	printf("MOVE, r%i <- %i", 
	       entry.arguments[0].reg, arg_get_value(m, entry.arguments[1]));
      } else if (entry.arguments[0].type == ADDRESS) {
	addr = mem_get_addr(m, entry.arguments[0]);
	m->mem[addr].data = arg_get_value(m, entry.arguments[1]);
	printf("MOVE, mem[%i] <- %i\n",
	       addr, arg_get_value(m, entry.arguments[1]));
      } else {
	  raise(m, "Inappropriate destination for move");
      }
      break;
    case LOAD:
      if (entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == ADDRESS) {
	  addr = mem_get_addr(m, entry.arguments[1]);
	  m->R[entry.arguments[0].reg] = mem_read(m, addr);
	  printf("LOAD, r%i <- %i\n", 
		 entry.arguments[1].reg, mem_read(m, addr));
      } else {
	  raise(m, "Inappropriate destination for load");
      }
      break;
    case STORE:
      if (entry.arguments[0].type == ADDRESS
	  && entry.arguments[1].type == REGISTER) {
	  addr = mem_get_addr(m, entry.arguments[0]);
	  mem_write(m, addr, entry.arguments[1].reg);
	  printf("STORE, mem[%i] <- %i\n", 
		 addr, m->R[entry.arguments[1].reg]);
      } else {
	  raise(m, "Inappropriate destination for store");
      }
      break;
    case IDM:
      if (entry.arguments[1].type == NUMBER) {
	if (entry.arguments[0].type == REGISTER) {
	  m->R[entry.arguments[0].reg] = entry.arguments[1].number;
	  printf("IDM, r%i <- %i\n", entry.arguments[0].reg, entry.arguments[1].number);
	} else if (entry.arguments[0].type == ADDRESS) {
	  addr = mem_get_addr(m, entry.arguments[0]);
	  mem_write(m, addr, entry.arguments[1].number);
	  printf("IDM, mem[%i] <- %i\n", addr, entry.arguments[1].number);
	} else {
	  raise(m, "Inappropriate destination for immediate data move");
	}
      } else {
	printf("%i\n", entry.arguments[0].type);
	raise(m, "Inappropriate number for immediate data move");
      }

      break;
    case EQ:
      printf("EQ\n");
      break;
    case NEQ:
      printf("NEQ\n");
      break;
    case LT:
      printf("LT\n");
      break;
    case LTE:
      printf("LTE\n");
      break;
    case AND:
      printf("AND\n");
      break;
    case OR:
      printf("OR\n");
      break;
    case NOT:
      printf("NOT\n");
      break;
    case ADD:
      printf("ADD\n");
      break;
    case SUB:
      printf("SUB\n");
      break;
    case MULT:
      printf("MULT\n");
      break;
    case DIV:
      printf("DIV\n");
      break;
    case NEG:
      printf("NEG\n");
      break;
    default:
      printf("Unknown opcode\n");
    }

    m->PC = m->nPC;

    //ensures only count instructions are executed
    if (count > 0 && --count == 0)
      return -RUN_OK;
  }

}


int run(struct ami_machine* m, int count)
{
  int ret = _run(m, count);


  if (ret == -RUN_BREAKPOINT) {
    skip_breakpoint();
  }
  //m->elapsed += nsec;

  return ret;
}

void show_exit_status(struct ami_machine *m)
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

