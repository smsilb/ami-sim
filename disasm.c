// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <ctype.h>

#include "sim.h"

void dump_registers(struct mips_machine *m) {
  printf("dumping registers\n");
}


void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst)
{
  printf("dumping disassembly\n");
}

stack_entry disasm_instr(char *instr) {
  stack_entry ret;
  char *token = strtok(instr, " ");
  
  //strip line number from instruction
  if (isdigit(token[0])) {
    token = strtok(NULL, " ");
  }

  switch(token[0]) {
  case 'h':
    ret.op = HALT;
    break;
  case 'w':
    ret.op = WRITE;
    break;
  }
}
