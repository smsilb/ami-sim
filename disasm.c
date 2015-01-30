// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"

void dump_registers(struct ami_machine *m) {
  printf("dumping registers\n");
}


void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst)
{
  printf("dumping disassembly\n");
}

struct stack_entry disasm_instr(char *instr) {
  struct stack_entry ret;
  char *copy = (char*) malloc(strlen(instr) + 1);

  //copy the instruction into a temp variable to preserve
  //for printing later
  ret.instruction = (char*) malloc(strlen(instr) + 1);
  strcpy(ret.instruction, instr);
  strcpy(copy, instr);
  char *token = strtok(copy, " ");


  
  //strip line number from instruction
  if (isdigit(token[0])) {
    token = strtok(NULL, " ");
  }

  switch(token[0]) {
  case 'h':
    if (!strcmp(token, "halt")) 
      ret.op = HALT;
    break;
  case 'w':
    if (!strcmp(token, "write"))
      ret.op = WRITE;
    break;
  case 'r':
    if (!strcmp(token, "read_boolean"))
      ret.op = READB;
    else if (!strcmp(token, "read_integer"))
      ret.op = READI;
    else 
      ret.op = MOVRTR;
    break;
  case 'p':
    if (!strcmp(token, "pc")) {
      //skip ':=' 
      /* token = strtok(NULL, " ");
      token = strtok(NULL, " ");

      if (token[0] == 'b') {
	ret.arguments[0].adBase = 0;
      } else if (token[0] == 'r') {
	//copy all but the first character to get
	//the register's number
	char* register_num;
	strncpy(register_num, token+1, strlen(token) - 1);
	ret.arguments[0].adBase = atoi(register_num);
      }
      
      token = strtok(NULL, " ");
      ret.arguments[0].adDisp = 0;

      //this will later need to be changed to handle more than just
      //numbers
      while (token != NULL && strcmp(token, "if")) {
	ret.arguments[0].adDisp += atoi(token);
	token = strtok(NULL, " ");
      }
      
      if (token == NULL) {
	ret.op = JUMP;
	ret.argc = 1;
      } else {
	token = strtok(NULL, " ");
	
	if (!strcmp(token, "not")) {
	  ret.op = JUMPNIF;
	} else {
	  ret.op = JUMPIF;
	}

	token = strtok(NULL, " ");

	if (token[1] == 'b') {
	  ret.arguments[1].adBase = 0;
	} else if (token[1] == 'r') {
	  //copy all but the first character to get
	  //the register's number
	  char* register_num;
	  strncpy(register_num, token+1, strlen(token) - 1);
	  ret.arguments[1].adBase = atoi(register_num);
	}
      
	token = strtok(NULL, " ");
	ret.arguments[1].adDisp = 0;

	//this will later need to be changed to handle more than just
	//numbers
	while (token != NULL && strcmp(token, "if")) {
	  ret.arguments[1].adDisp += atoi(token);
	  token = strtok(NULL, " ");
	}
	
	ret.argc = 2;
      }*/
      ret.op = JUMP;
      break;
    }
  case 'b':
    ret.op = MOVRTR;
    break;
  case 'c':
    ret.op = MOVATA;
    break; 
  default:
    printf("Unrecognized command: %s\n", token);
    exit(1);
  }

  return ret;
}
