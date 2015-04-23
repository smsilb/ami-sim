// Copyright (c) 2015, Sam Silberstein. All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: smsilb14@g.holycross.edu

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
#include <sys/shm.h>
#include <sys/ipc.h>

#define MAX_ARGS 5
#define MAX_ARGLEN 30
#define MAX_BREAKPOINTS 100

static int ac;
static char av[MAX_ARGS][MAX_ARGLEN];

int strpcmp(char *shortstring, char *longstring) {
  return strncmp(shortstring, longstring, strlen(shortstring));
}


int add_breakpoint(struct ami_machine *m, unsigned int addr) {
  static int bpnum = 1;
  struct breakpoint *b = malloc(sizeof(struct breakpoint));
  b->id = bpnum++;
  b->enabled = 1;
  b->addr = addr;
  b->next = m->breakpoints;
  m->breakpoints = b;
  return b->id;
}

void del_breakpoint(struct ami_machine *m, unsigned int id) {
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

int is_breakpoint(struct ami_machine *m, unsigned int addr) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (!b) return 0;
  if (b->enabled) return 1;
  printf("skipping breakpoint at %d\n", addr);
  b->enabled = 1;
  return 0;
}

int find_breakpoint(struct ami_machine *m, unsigned int addr) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (!b) return 0;
  return b->id;
}

void skip_breakpoint(struct ami_machine *m) {
  unsigned int addr = m->PC;
  struct breakpoint *b = m->breakpoints;
  while (b != NULL && b->addr != addr)
    b = b->next;
  if (b) b->enabled = 0;
}

void unskip_breakpoints(struct ami_machine *m) {
  struct breakpoint *b = m->breakpoints;
  while (b != NULL) {
    b->enabled = 1;
    b = b->next;
  }
}

void dump_breakpoints(struct ami_machine *m) {
  struct breakpoint *b = m->breakpoints;
  if (b == NULL) printf("no breakpoints set\n");
  while (b != NULL) {
    printf("breakpoint %d at address %d\n", b->id, b->addr);
    b = b->next;
  }
}

void readcmd(struct ami_machine *m) {
  static char *line = NULL;
  if (line) free(line);
  if (m->opt_graphical == 1) {
    char buffer[80];
    int buffered = 0;
    
    while (*m->shm == '\0') {
      nanosleep(100000, &buffered);
    }
 
    char *s;

    for (s = m->shm; *s != '\0'; s++) {
      buffer[buffered] = *s;
      buffered++;
    }
    buffer[buffered] = '\0';

    line = (char *) malloc(buffered);
    strcpy(line, buffer);
    printf("> %s\n", line);

    int i;
    s = m->shm;
    for (i = 0; i < buffered; i++) {
      *s = '\0';
      s++;
    }

    buffered = 0;
  } else {
    line = readline("> ");
    if (!line) {
      ac = 1;
      strcpy(av[0], "q");
      return;
    } else if (!*line) {
      return; // do same command again
    }
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

void send_string_to_gui(struct ami_machine *m, char string[]) {
  char *s;
  int i, sleep_time;
  //copy string to shared memory
  s = m->shm + 1;
  for (i = 0; i < strlen(string); i++) {
    *s = string[i];
    s++;
  }
  *s = '\0';

  //set 'ready' flag
  *m->shm = 'r';

  while (*m->shm != '\0') {
    nanosleep(100000, &sleep_time);
  }
}

void update_gui(struct ami_machine *m) {
  int i, buffer_size = 256, sent_chars = 0, sleep_time;
  char *s;
  char buffer[buffer_size];

  shmdt(m->shm);

  int shmid;
  if ((shmid = shmget((key_t) 1234, 256, IPC_CREAT | 0666)) < 0) {
    perror("Could not initialize GUI\n");
    exit(1);
  }

  if ((m->shm = shmat(shmid, NULL, 0)) == (char *) -1) {
    perror("Could not initialize GUI\n");
    exit(1);
  }
  
  s = m->shm;
  for (i = 0; i < 256; i++) {
    *s = '\0';
    s++;
  }

  //build string to send
  sprintf(buffer, "pc: %i\nb: %i\n", m->PC, m->R[0]);
  for (i = 1; i < m->reg_count; i++) {
    if (strlen(buffer) + 5 > 255) {
      break;
    } else {
      //add new line to string
      sprintf(buffer, "%s%i: %i\n", buffer, i, m->R[i]);
    }
  }

  //add delimiter to end of buffer
  sprintf(buffer, "%s~", buffer);

  for (i = 0; i < STACK_SIZE; i++) {
    char *data = read_stack_entry(m, i);
    if (strlen(buffer) + strlen(data) > 253) {
      send_string_to_gui(m, buffer);
      sprintf(buffer, "%s\n", data);
    } else {
      sprintf(buffer, "%s%s\n", buffer, data); 
    }
    free(data);
  }

  if (m->console_io_status == 1) {
      if (strlen(buffer) + 2 > 253) {
          send_string_to_gui(m, buffer);
          sprintf(buffer, "~>");
      } else {
          sprintf(buffer, "%s~>", buffer);
      }
  } else if (m->console_io_status == 2) {
      send_string_to_gui(m, buffer);
      sprintf(buffer, "~-> %i", m->console_io_value);
      m->console_io_status = 0;
  }

  send_string_to_gui(m, buffer);

  //set 'end' flag
  *m->shm = 'r';
  *(m->shm + 1) = 'e';

  //if we were waiting for input, capture the input sent from the gui
  if (m->console_io_status == 1) {
      while (*m->shm != 'i') {
          //printf("Waiting for input\n");
          nanosleep(100000);
      }
      m->console_io_value = atoi(m->shm + 1);
      m->console_io_status = 0;
  }

  /*for (i = 1; i < m->reg_count; i++) {
    if (strlen(buffer) + 5 > 256) {
      strcpy(m->shm, buffer);
      while (*m->shm != '*') {
	nanosleep(100000, &sleep_time);
      }
      sprintf(buffer, "%i: %i\n", i, m->R[i]);
    } else {
      sprintf(buffer, "%s%i: %i\n", buffer, i, m->R[i]);
    }
  }
  strcpy(m->shm, buffer);
  /*  while (*m->shm != '*') {
    nanosleep(100000, &sleep_time);
  }
  strcpy(m->shm, "end");*/

  //printf("Sent %s\n", m->shm);
  /*  while (*m->shm != '*') {
    nanosleep(100000, &sleep_time);
    }*/

  *m->shm = '\0';
}

void interactive_debug(struct ami_machine *m)
{
  rl_initialize();
  rl_bind_key('\t', rl_insert);

  int err;
  if (m->opt_graphical) {
    update_gui(m);
  }
  printf("Welcome to the AMI simulator built-in debugger. Type 'help' for a listing of commands.");
  printf("\n");

  for (;;) {
    
      if (m->opt_dumpreg) {
	dump_registers(m);
      }
      if (m->opt_printstack != -1) {
	dump_stack(m, m->opt_printstack);
      } 

    err = 0;

    readcmd(m);
    if (ac == 0)
      continue;
    if (!strpcmp(av[0], "quit") || !strpcmp(av[0], "exit")) {
      printf("exiting debugger\n");
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
      m->halted = 0;
      free_segments(m);
      allocate_stack(m);
      
    } else if (!strpcmp(av[0], "breakpoint")) {
      if (ac != 2) {
	printf("expected an address, but got %d arguments\n", ac-1);
      } else {
	unsigned int addr = atoi(av[1]);
	if (addr == 0)
	  printf("expected an address, but got '%s' instead\n", av[1]);
	else {
	  int id = add_breakpoint(m, addr);
	  printf("set breakpoint %d at address %d\n", id, addr);
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
	if (size < 0) printf("expected a positive integer, but got '%s' instead\n", av[2]);
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
	  m->opt_printstack = -1;
	}
      } else {
	printf("don't know how to display '%s'; try help\n", av[1]);
      }
    } else if (!strpcmp(av[0], "?") || !strpcmp(av[0], "help")) {
      printf(
	  "quit               -- quit the debugger\n"
	  "continue           -- continue running the program until exit or breakpoint\n"
	  "step               -- execute one step of the program\n"
	  "step <n>           -- execute n steps of the program\n"
	  "reset	      -- reset the simulation state and restart execution of the program from the beginning\n"
	  "break <addr>       -- set a breakpoint to occur after execution reaches <addr>\n"
	  "delete <i>         -- delete the breakpoint <i>\n"
	  "info <thing>       -- get info about <thing>, which can be 'breakpoints', 'stack', or 'registers'\n"
	  "display <thing>    -- periodically display <thing>, which can be 'stack', or 'registers'\n"
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
    if (m->opt_graphical) {
      update_gui(m);
    }
  }

}
