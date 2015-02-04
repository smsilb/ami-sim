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
  char *token = strtok(instr, " ");


  
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
    else {
      //skip ':='
      token = strtok(NULL, " ");
      
      token = strtok(NULL, " ");
      
      if (token[0] == 'c')
	ret.op = LOAD;
      else if (token[0] == 'r')
	ret.op = MOVRTR;
      else if (isdigit(token[0]))
	ret.op = IDM;
      else if (token[0] == '-')
	ret.op = IDMN;
      break;
    }
    break;
  case 'p':
    if (!strcmp(token, "pc")) {
      //skip ':=' 
      token = strtok(NULL, " ");
      
      token = read_argument(&ret, "if");

      /*if (token[0] == 'b') {
	ret.arguments[0].adBase = 0;
      } else if (token[0] == 'r') {
	//copy all but the first character to get
	//the register's number
	char* register_num;
	strncpy(register_num, token+1, strlen(token) - 1);
	ret.arguments[0].adBase = atoi(register_num);
      }
      */
      
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

	token = read_argument(&ret, "");

	/*token = strtok(NULL, " ");

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
	while (token != NULL) {
	  ret.arguments[1].adDisp += atoi(token);
	  token = strtok(NULL, " ");
	}
	*/
	ret.argc = 2;
      }
      break;
    }
  case 'b':
    //skip ':='
    token = strtok(NULL, " ");

    token = strtok(NULL, " ");

    if (token[0] == 'c')
      ret.op = LOAD;
    else if (token[0] == 'r')
      ret.op = MOVRTR;
    else if (isdigit(token[0]))
      ret.op = IDM;
    else if (token[0] == '-')
      ret.op = IDMN;
    break;
  case 'c':
    //read in the address
    read_argument(&ret, ":=");

    token = strtok(NULL, " ");

    if (token[0] == 'c')
      ret.op = MOVATA;
    else if (token[0] == 'r')
      ret.op = STORE;
    else if (isdigit(token[0]))
      ret.op = IDM;
    else if (token[0] == '-')
      ret.op = IDMN;
    break; 
  default:
    printf("Unrecognized command: %s\n", token);
    exit(1);
  }

  //regardless of which case it is, they are all instructions
  ret.data_type = INSTRUCTION;

  return ret;
}

char* read_argument(struct stack_entry *ret, char *next_token) {
  //this function assumes that it is being called from
  //inside disasm_instruction, and that strtok has been called
  //on the instruction string
  char *token = strtok(NULL, " ");

  //loop until end of line, or the next token is arrived at
  while (token != NULL && strcmp(token, next_token)) {
    //stubbed until full parse is working
    token = strtok(NULL, " ");
  }

  return token;
}
