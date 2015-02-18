// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "sim.h"

void dump_registers(struct ami_machine *m) {
  printf("Registers:\n");
  printf("b: %i\n", m->R[0]);

  int i;
  for (i = 1; i < m->reg_count; i++) {
    printf("%i: %i\n", i, m->R[i]);
  }
}


void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst)
{
  printf("dumping disassembly\n");
}

struct stack_entry disasm_instr(struct ami_machine *m, char *instr) {
  struct stack_entry ret;
  char *stop_words[12];
  char *copy = (char*) malloc(strlen(instr) + 1);

  //copy the instruction to preserve
  //for printing later
  ret.instruction = (char*) malloc(strlen(instr) + 1);
  strcpy(ret.instruction, instr);
  char *token = strtok(instr, " ");
  ret.argc = 0;


  init_stop_words(stop_words);
  
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
    if (!strcmp(token, "write")) {
      ret.op = WRITE;
      read_argument(&ret, strtok(NULL, " "), stop_words, 12);
    }
    break;
  case 'r':
    if (!strcmp(token, "read_boolean")) {
      ret.op = READB;
      read_argument(&ret, strtok(NULL, " "), stop_words, 12);
    } else if (!strcmp(token, "read_integer")) {
      ret.op = READI;
      read_argument(&ret, strtok(NULL, " "), stop_words, 12);
    } else {
      ret.arguments[0].reg = atoi(token + 1);
      ret.arguments[0].type = REGISTER;
      ret.argc = 1;

      //check if this is setting aside a new register
      //and increase register count
      if (ret.arguments[0].reg > m->reg_count) {
	m->reg_count = ret.arguments[0].reg + 1;
      }

      //skip ':='
      token = strtok(NULL, " ");

      token = strtok(NULL, " ");
      
      if (isdigit(token[0])) {
	ret.op = IDM;
	read_argument(&ret, token, stop_words, 12);
      } else if (token[0] == '-') {
	if (strlen(token) == 1) {
	  token = strtok(NULL, " ");
	
	  if (isdigit(token[0])) {
	    ret.op = IDM;
	    ret.arguments[1].number = -1 * atoi(token);
	  } else {
	    ret.op = NEG;
	    read_argument(&ret, token, stop_words, 12);
	  }
	} else {
	  if (isdigit(token[1])) {
	    ret.op = IDM;
	    ret.arguments[1].number = -1 * atoi(token);
	  } else {
	    ret.op = NEG;
	    read_argument(&ret, token + 1, stop_words, 12);
	  }
	}
      } else {
	//Argument arithmetic
	
	if (!strcmp(token, "not")) {
	  ret.op = NOT;
	  read_argument(&ret, strtok(NULL, " "), stop_words, 12);
	} else {
	  /*
	   * Either ALU arithmetic, Argument arithmetic,
	   * MOVE, or LOAD.
	   * Will need to check after parsing that ALU ops
	   * only used with registers as arguments.
	   */
	  char argType = token[0];
	  //	  if (argType == 'c') {
	  token = read_argument(&ret, token, stop_words, 12);
	    //	  } 
	  if (argType == 'r' || argType == 'b') {
	    token = strtok(NULL, " ");
	  }
	  

	  if (token == NULL) {
	    if (argType == 'c') {
	      ret.op = LOAD;
	    } else if (argType == 'r' || argType == 'b') {
	      ret.op = MOVE;
	    }
	  } else {
	    if (!strcmp(token, "=")) {
	      ret.op = EQ;
	    } else if (!strcmp(token, "/=")) {
	      ret.op = NEQ;
	    } else if (!strcmp(token, "<")) {
	      ret.op = LT;
	    } else if (!strcmp(token, "<=")) {
	      ret.op = LTE;
	    } else if (!strcmp(token, "and")) {
	      ret.op = AND;
	    } else if (!strcmp(token, "or")) {
	      ret.op = OR;
	    } else if (!strcmp(token, "+")) {
	      ret.op = ADD;
	    } else if (!strcmp(token, "-")) {
	      ret.op = SUB;
	    } else if (!strcmp(token, "*")) {
	      ret.op = MULT;
	    } else if (!strcmp(token, "/")) {
	      ret.op = DIV;
	    } 
	    read_argument(&ret, strtok(NULL, " "), stop_words, 12);
	  }
	}
      }
      break;
    }
    break;
  case 'p':
    if (!strcmp(token, "pc")) {
      //skip ':=' 
      strtok(NULL, " ");
      token = strtok(NULL, " ");
      
      token = read_argument(&ret, token, stop_words, 12);
      token = strtok(NULL, " ");

      if (token == NULL) {
	ret.op = JUMP;
	ret.argc = 1;
      } else {
	token = strtok(NULL, " ");
	if (!strcmp(token, "not")) {
	  ret.op = JUMPNIF;
	  token = strtok(NULL, " ");
	} else {
	  ret.op = JUMPIF;
	}
	
	token = read_argument(&ret, token, stop_words, 12);

	ret.argc = 2;
      }
      break;
    }
  case 'b':
    ret.arguments[0].type = REGISTER;
    ret.arguments[0].reg = 0;
    ret.argc = 1;
    //skip ':='
    token = strtok(NULL, " ");

    token = strtok(NULL, " ");

    if (token[0] == 'c') {
      ret.op = LOAD;
    } else if (token[0] == 'r') {
      ret.op = MOVE;
    } else if (isdigit(token[0])) {
      ret.op = IDM;
    } else if (token[0] == '-') {
      ret.op = IDM;
    }

    read_argument(&ret, token, stop_words, 12);

    break;
  case 'c':
    //read in the address
    read_argument(&ret, token, stop_words, 12);

    token = strtok(NULL, " ");

    if (token[0] == 'c') {
      ret.op = MOVE;
    } else if (token[0] == 'r') {
      ret.op = STORE;
    } else if (isdigit(token[0])) {
      ret.op = IDM;
    } else if (token[0] == '-') {
      ret.op = IDM;
    }
    read_argument(&ret, token, stop_words, 12);
    break; 
  default:
    printf("Unrecognized command: %s\n", token);
    exit(1);
  }

  //regardless of which case it is, they are all instructions
  ret.data_type = INSTRUCTION;

  return ret;
}

int contains(char *needle, char *haystack[], int size) {
  int i;
  for (i = 0; i < size; i++) {
    if (!strcmp(haystack[i], needle)) {
      return TRUE;
    }
  }

  return FALSE;
}

char* read_argument(struct stack_entry *ret, char *token, char * stop_words[], int words) {
  if (ret->argc >= 3) {
    //raise("Too many arguments in instruction\n");
    printf("Too many arguments in instruction\n");
  }

  //get the index of this argument
  int argNum = ret->argc;
  ret->argc++;
  
  //this function assumes that it is being called from
  //inside disasm_instruction, and that strtok has been called
  //on the instruction string

  if (token[0] == 'c' || token[strlen(token) - 1] == ',') {
    ret->arguments[argNum].type = ADDRESS;
    int addc= 0;

    //loop until end of line, or the next token is arrived at
    while (token != NULL && !contains(token, stop_words, words)) {
      if (token[0] == 'r' || token[0] == 'b') {
	//this address is a register
	ret->arguments[argNum].add[addc].type = REG;
	ret->arguments[argNum].add[addc].value = atoi(token + 1);
	addc += 1;
      } else if (isdigit(token[0])) {
	//this address is a displacement
	ret->arguments[argNum].add[addc].type = DISP;
	ret->arguments[argNum].add[addc].value = atoi(token);
	addc += 1;
      }
      token = strtok(NULL, " ");
    }
    ret->arguments[argNum].addc = addc;
   } else if (token[0] == 'r') {
    //if this argument is a register, read a number into it
    ret->arguments[argNum].type = REGISTER;
    ret->arguments[argNum].reg = atoi(token + 1);
  } else if (token[0] == 'b') {
    ret->arguments[argNum].type = REGISTER;
    ret->arguments[argNum].reg = 0;
  } else if (isdigit(token[0])) {
    ret->arguments[argNum].type = NUMBER;
    ret->arguments[argNum].number = atoi(token);
  } else {
    //raise("Inappropriate argument: %s\n", token);
    printf("Inappropriate argument: %s\n", token);
    token = strtok(NULL, " ");
  }

  return token;
}

void init_stop_words(char *stop_words[]) {
  stop_words[0] = (char *) malloc(sizeof("if") + 1);
  strcpy(stop_words[0], "if");

  stop_words[1] = (char *) malloc(sizeof("and") + 1);
  strcpy(stop_words[1], "and");

  stop_words[2] = (char *) malloc(sizeof("or") + 1);
  strcpy(stop_words[2], "or");

  stop_words[3] = (char *) malloc(sizeof("+") + 1);
  strcpy(stop_words[3], "+");

  stop_words[4] = (char *) malloc(sizeof("-") + 1);
  strcpy(stop_words[4], "-");

  stop_words[5] = (char *) malloc(sizeof("*") + 1);
  strcpy(stop_words[5], "*");

  stop_words[6] = (char *) malloc(sizeof("/") + 1);
  strcpy(stop_words[6], "/");

  stop_words[7] = (char *) malloc(sizeof(":=") + 1);
  strcpy(stop_words[7], ":=");

  stop_words[8] = (char *) malloc(sizeof("=") + 1);
  strcpy(stop_words[8], "=");

  stop_words[9] = (char *) malloc(sizeof("/=") + 1);
  strcpy(stop_words[9], "/=");
  
  stop_words[10] = (char *) malloc(sizeof("<") + 1);
  strcpy(stop_words[10], "<");
  
  stop_words[11] = (char *) malloc(sizeof("<=") + 1);
  strcpy(stop_words[11], "<=");
}
