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
  int op, addr1, addr2;
  char str[20];
  struct stack_entry entry;
  for (;;) {
    if (m->halted)
      return -RUN_HALTED;

    if (is_breakpoint(m, m->PC)) {
      return -RUN_BREAKPOINT;
    }

    m->nPC = m->PC + 1;

    op = m->mem[m->PC].op;
    entry = m->mem[m->PC];
    printf("%s\n", entry.instruction);

    switch(op) {
    case HALT:
      printf("HALT\n");
      m->halted = 1;
      break;
    case WRITE:
      printf("WRITE -> %i\n", arg_get_value(m, entry.arguments[0]));
      break;
    case READB:
      if (entry.arguments[0].type == ADDRESS) {
	addr1 = mem_get_addr(m, entry.arguments[0]);
	fgets(str, 20, stdin);
	mem_write(m, addr1, atoi(str) != 0);
	printf("READB, mem[%i] <- %i\n", addr1, atoi(str) != 0);
      } else {
	raise(m, "Non address destination for READB");
      }
      break;
    case READI:
      if (entry.arguments[0].type == ADDRESS) {
	addr1 = mem_get_addr(m, entry.arguments[0]);
	fgets(str, 20, stdin);
	mem_write(m, addr1, atoi(str));
	printf("READI, mem[%i] <- %i\n", addr1, atoi(str));
      } else {
	raise(m, "Non address destination for READI");
      }
      break;
    case JUMP:
      addr1 = add_get_value(m, entry.arguments[0]);
      m->nPC = addr1;
      printf("JUMP to %i\n", addr1);
      break;
    case JUMPIF:
      addr1 = add_get_value(m, entry.arguments[0]);
      if (arg_get_value(m, entry.arguments[1])) {
	m->nPC = addr1;
	printf("JUMPIF to %i, COND TRUE\n", addr1);
      } else {
	printf("JUMPIF to %i, COND FALSE\n", addr1);
      }
      break;
    case JUMPNIF:
      addr1 = add_get_value(m, entry.arguments[0]);
      if (arg_get_value(m, entry.arguments[1]) == 0) {
	m->nPC = addr1;
	printf("JUMPNIF to %i, COND TRUE\n", addr1);
      } else {
	printf("JUMPNIF to %i, COND FALSE\n", addr1);
      }
      break;
    case MOVE:
      if (entry.arguments[0].type == REGISTER) {
	m->R[entry.arguments[0].reg] = arg_get_value(m, entry.arguments[1]);
	printf("MOVE, r%i <- %i", 
	       entry.arguments[0].reg, arg_get_value(m, entry.arguments[1]));
      } else if (entry.arguments[0].type == ADDRESS) {
	addr1 = mem_get_addr(m, entry.arguments[0]);
	mem_write(m, addr1,  arg_get_value(m, entry.arguments[1]));
	printf("MOVE, mem[%i] <- %i\n",
	       addr1, arg_get_value(m, entry.arguments[1]));
      } else {
	  raise(m, "Inappropriate destination for move");
      }
      break;
    case LOAD:
      if (entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == ADDRESS) {
	addr1 = mem_get_addr(m, entry.arguments[1]);
	m->R[entry.arguments[0].reg] = mem_read(m, addr1);
	printf("LOAD, r%i <- %i\n", 
		 entry.arguments[0].reg, mem_read(m, addr1));
      } else {
	raise(m, "Inappropriate destination for load");
      }
      break;
    case STORE:
      if (entry.arguments[0].type == ADDRESS
	  && entry.arguments[1].type == REGISTER) {
	  addr1 = mem_get_addr(m, entry.arguments[0]);
	  mem_write(m, addr1, entry.arguments[1].reg);
	  printf("STORE, mem[%i] <- %i\n", 
		 addr1, m->R[entry.arguments[1].reg]);
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
	  addr1 = mem_get_addr(m, entry.arguments[0]);
	  mem_write(m, addr1, entry.arguments[1].number);
	  printf("IDM, mem[%i] <- %i\n", addr1, entry.arguments[1].number);
	} else {
	  raise(m, "Inappropriate destination for immediate data move");
	}
      } else {
	printf("%i\n", entry.arguments[0].type);
	raise(m, "Inappropriate number for immediate data move");
      }

      break;
    case EQ:
      if (entry.argc == 3
	  && entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == REGISTER
	  && entry.arguments[2].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) ==
	    arg_get_value(m, entry.arguments[2])) {
	  m->R[addr1] = 1;
	  printf("EQ, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("EQ, r%i <- false\n", addr1);
	}
      } else {
	raise(m, "Non register argument in EQ instruction");
      } 
      break;
    case NEQ:
      if (entry.argc == 3
	  && entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == REGISTER
	  && entry.arguments[2].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) ==
	    arg_get_value(m, entry.arguments[2])) {
	  m->R[addr1] = 1;
	  printf("NEQ, r%i <- false\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("NEQ, r%i <- true\n", addr1);
	}
      } else {
	raise(m, "Non register argument in NEQ instruction");
      }
      break;
    case LT:
      if (entry.argc == 3
	  && entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == REGISTER
	  && entry.arguments[2].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) <
	    arg_get_value(m, entry.arguments[2])) {
	  m->R[addr1] = 1;
	  printf("LT, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("LT, r%i <- false\n", addr1);
	}
      } else {
	raise(m, "Non register argument in LT instruction");
      }
      break;
    case LTE:
      if (entry.argc == 3
	  && entry.arguments[0].type == REGISTER
	  && entry.arguments[1].type == REGISTER
	  && entry.arguments[2].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) <=
	    arg_get_value(m, entry.arguments[2])) {
	  m->R[addr1] = 1;
	  printf("LTE, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("LTE, r%i <- false\n", addr1);
	}
      } else {
	raise(m, "Non register argument in LTE instruction");
      }
      break;
    case AND:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER){
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) != 0
	    && arg_get_value(m, entry.arguments[2]) != 0) {
	  m->R[addr1] = 1;
	  printf("AND, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("AND, r%i <- false\n", addr1);
	} 
      } else { 
	raise(m, "Non register destination for AND");
      }
      break;
    case OR:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER){
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) != 0
	    || arg_get_value(m, entry.arguments[2]) != 0) {
	  m->R[addr1] = 1;
	  printf("OR, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("OR, r%i <- false\n", addr1);
	}
      } else { 
	raise(m, "Non register destination for OR");
      }
      break;
    case NOT:
      if (entry.argc == 2 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[1]) == 0) {
	  m->R[addr1] = 1;
	  printf("NOT, r%i <- true\n", addr1);
	} else {
	  m->R[addr1] = 0;
	  printf("NOT, r%i <- false\n", addr1);
	}
      } else { 
	raise(m, "Non register destination for NOT");
      }
      break;
    case ADD:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
				       + arg_get_value(m, entry.arguments[2]);
	printf("ADD, r%i <- %i\n", addr1, m->R[addr1]);
      } else { 
	raise(m, "Non register destination for ADD");
      }
      break;
    case SUB:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
	  - arg_get_value(m, entry.arguments[2]);
	printf("SUB, r%i <- %i\n", addr1, m->R[addr1]);
      } else { 
	raise(m, "Non register destination for SUB");
      }
      break;
    case MULT:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
	  * arg_get_value(m, entry.arguments[2]);
	printf("MULT, r%i <- %i\n", addr1, m->R[addr1]);
      } else { 
	raise(m, "Non register destination for MULT");
      }
      break;
    case DIV:
      if (entry.argc == 3 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	if (arg_get_value(m, entry.arguments[2]) == 0) {
	  raise(m, "Division by zero");
	} else {
	  m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
	    / arg_get_value(m, entry.arguments[2]);
	  printf("DIV, r%i <- %i\n", addr1, m->R[addr1]);
	}
      } else { 
	raise(m, "Non register destination for DIV");
      }
      break;
    case NEG:
      if (entry.argc == 2 && entry.arguments[0].type == REGISTER) {
	addr1 = entry.arguments[0].reg;
	m->R[addr1] = -1 * arg_get_value(m, entry.arguments[1]);
	printf("NEG, r%i <- %i\n", addr1, m->R[addr1]);
      } else {
	raise(m, "Non register destination for NEG");
      }
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
  } else if (ret == -RUN_HALTED) {
    printf("Program is halted\n");
  }

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

