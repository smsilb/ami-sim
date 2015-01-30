// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_READLINE
#include <readline.h>
#else
#ifdef USE_EDITLINE
#include <editline/readline.h>
#error balk
#else
#include "readline.h"
#endif
#endif

#include "sim.h"

#define MAX_ARGS 5
#define MAX_ARGLEN 30
#define MAX_BREAKPOINTS 100

static int ac;
static char av[MAX_ARGS][MAX_ARGLEN];

int strpcmp(char *shortstring, char *longstring) {
  return strncmp(shortstring, longstring, strlen(shortstring));
}


int add_breakpoint(struct mips_machine *m, unsigned int addr) {
  static int bpnum = 1;
  struct breakpoint *b = malloc(sizeof(struct breakpoint));
  b->id = bpnum++;
  b->enabled = 1;
  b->addr = addr;
  b->next = m->breakpoints;
  m->breakpoints = b;
  return b->id;
}

void del_breakpoint(struct mips_machine *m, unsigned int id) {
  struct breakpoint **pprev = &m->breakpoints;
  struct breakpoint *b = *pprev;
  while (b != NULL && b->id != id) {
    pprev = &b->next;
    b = *pprev;
  }
  if (b != NULL) {
    *pprev = b->next;
    free(b);
    printf("breakpoint %d deleted\n", id);
  } else {
    printf("no such breakpoint %d\n", id);
  }
}

int is_breakpoint(struct mips_machine *m, unsigned int addr) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (!b) return 0;
  if (b->enabled) return 1;
  printf("skipping breakpoint at 0x%08x\n", addr);
  b->enabled = 1;
  return 0;
}

int find_breakpoint(struct mips_machine *m, unsigned int addr) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (!b) return 0;
  return b->id;
}

void skip_breakpoint(struct mips_machine *m) {
  unsigned int addr = m->PC;
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (b) b->enabled = 0;
}

void unskip_breakpoints(struct mips_machine *m) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL) {
    b->enabled = 1;
    b = b->next;
  }
}

void dump_breakpoints(struct mips_machine *m) {
  struct breakpoint *b = m->breakpoints;
  if (b == NULL) printf("no breakpoints set\n");
  while (b != NULL) {
    printf("breakpoint %d at address 0x%08x\n", b->id, b->addr);
    b = b->next;
  }
}

void readcmd() {
  static char *line = NULL;
  if (line) free(line);
  line = readline("> ");
  if (!line) {
    ac = 1;
    strcpy(av[0], "q");
    return;
  } else if (!*line) {
    return; // do same command again
  }

  add_history(line);

  char *tok = strtok(line, " ");
  if (tok == NULL) {
    free(line);
    return; // do same command again
  }

  ac = 0;
  while (tok != NULL && ac < MAX_ARGS) {
    strncpy(av[ac], tok, MAX_ARGLEN-1);
    av[ac++][MAX_ARGLEN-1] = '\0';
    tok = strtok(NULL, " ");
  }
}

void interactive_debug(struct mips_machine *m)
{
  rl_initialize();
  rl_bind_key('\t', rl_insert);

  int err;

  printf("Welcome to the MIPS simulator built-in debugger. Type 'help' for a listing of commands.");
  printf("\n");

  for (;;) {

    err = 0;

    readcmd();
    if (ac == 0)
      continue;
    if (!strpcmp(av[0], "quit") || !strpcmp(av[0], "exit")) {
      printf("exiting MIPS debugger\n");
      exit(0);
    } else if (!strpcmp(av[0], "continue")) {
      err = run(m, 0);
    } else if (!strpcmp(av[0], "step")) {
      int steps = (ac == 1 ? 1 : atoi(av[1]));
      if (steps <= 0)
	printf("expected a positive integer, but got '%s' instead\n", av[1]);
      else {
	err = run(m, steps);
      }
    } else if (!strpcmp(av[0], "reset")) {
      printf("Resetting program state\n");
      unskip_breakpoints(m);
      memset(m->R, 0, sizeof(m->R));
      m->PC = m->nPC = 0;
      free_segments(m);
      allocate_stack(m);
      push_arguments(m);
      
    } else if (!strpcmp(av[0], "breakpoint")) {
      if (ac != 2) {
	printf("expected an address in hex, but got %d arguments\n", ac-1);
      } else {
	unsigned int addr = strtoul(av[1], NULL, 16);
	if (addr == 0)
	  printf("expected an address in hex, but got '%s' instead\n", av[1]);
	else {
	  int id = add_breakpoint(m, addr);
	  printf("set breakpoint %d at address 0x%08x\n", id, addr);
	}
      }
    } else if (!strpcmp(av[0], "delete")) {
      if (ac != 2) {
	printf("exepcted a breakpoint number, but got %d arguments\n", ac-1);
      } else {
	int id = atoi(av[1]);
	if (id == 0)
	  printf("expected a breakpoint number, but got '%s' instead\n", av[1]);
	else
	  del_breakpoint(m, id);
      }
    } else if (!strpcmp(av[0], "info")) {
      if (ac == 1) {
	printf("expected an argument, one of: breakpoints, stack, memory, or registers\n");
      } else if (!strpcmp(av[1], "registers")) {
	dump_registers(m);
      } else if (!strpcmp(av[1], "stack")) {
	int size = m->opt_printstack ? m->opt_printstack : 64;
	if (ac > 2) size = atoi(av[2]);
	if (size <= 0) printf("expected a positive integer, but got '%s' instead\n", av[2]);
	else dump_stack(m, size);
      } else if (!strpcmp(av[1], "breakpoints")) {
	dump_breakpoints(m);
      } else if (!strpcmp(av[1], "memory")) {
	dump_segments(m);
      } else {
	printf("don't know any info about '%s'; try help\n", av[1]);
      }
    } else if (!strpcmp(av[0], "display") || !strpcmp(av[0], "undisplay")) {
      if (ac == 1) {
	printf("expected an argument, one of: stack, registers, or disassembly\n");
      } else if (!strpcmp(av[1], "registers")) {
	m->opt_dumpreg = (av[0][0] == 'd');
      } else if (!strpcmp(av[1], "stack")) {
	if (av[0][0] == 'd') {
	  int size = 64;
	  if (ac > 2) size = atoi(av[2]);
	  if (size <= 0) printf("expected a positive integer, but got '%s' instead\n", av[2]);
	  else m->opt_printstack = size;
	} else {
	  m->opt_printstack = 0;
	}
      } else {
	printf("don't know how to display '%s'; try help\n", av[1]);
      }
    } else if (!strpcmp(av[0], "?") || !strpcmp(av[0], "help")) {
      printf(
	  "quit               -- quit the MIPS debugger\n"
	  "continue           -- continue running the program until exit or breakpoint\n"
	  "step               -- execute one step of the program\n"
	  "step <n>           -- execute n steps of the program\n"
	  "reset	      -- reset the simulation state and restart execution of the program from the beginning\n"
	  "break <addr>       -- set a breakpoint to occur after execution reaches <addr>\n"
	  "delete <i>         -- delete the breakpoint <i>\n"
	  "info <thing>       -- get info about <thing>, which can be 'breakpoints', 'stack', 'memory', or 'registers'\n"
	  "display <thing>    -- periodically display <thing>, which can be 'stack', 'registers', or 'disassembly'\n"
	  "                      'stack' takes an optional argument of how many words to display;\n"
	  "undisplay <thing>  -- don't periodically display <thing> any more\n"
	  "[enter]            -- repeat the last command\n"
	  "\n"
	  "Note: All commands can be abbreviated by their first letter or any prefix.\n"
	  "For example, 'i b' stands for 'info breakpoints,'\n"
	  "and 's 10' stands for 'step 10'.\n"
	  );
    } else {
      printf("unrecognized command '%s'\n", av[0]);
    }
  }

}
