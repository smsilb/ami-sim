// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sim.h"

int main(int ac, char **av)
{
  int pfd1[2], pfd2[2], status;
  char *av0 = *(av++); ac--;

  struct ami_machine *m = create_ami_machine();
  m->opt_graphical = 1;
  m->reg_count = 1;
  m->opt_printstack = -1;

  if (ac <= 0) {
    printf("Usage: ./sim {FLAGS} FILENAME\n");
    exit(1);
  } else {
    if (ac > 1) {
      char *flag;
      for (ac; ac > 1; ac--) {
	flag = *(av++);

	if (flag[0] == '-') 
	  flag++;

	if (!strcmp(flag, "t")) {
	  m->opt_graphical = 0;
	}
      }
    }
  }

  m->filename = strdup(*av);

  printf("Filename: %s\n", m->filename);
  allocate_stack(m);

  if (m->opt_graphical == 1) {
    /*if(pipe(pfd1) == -1 || pipe(pfd2) == -1) {
      printf("Could not open pipe\n");
      exit(1);
    }

#define CHILDREAD pfd2[0]
#define CHILDWRITE pfd1[1]
#define PARENTREAD pfd1[0]
#define PARENTWRITE pfd2[1]
  
    m->childread = CHILDREAD;
    m->childwrite = CHILDWRITE;
    m->parentread = PARENTREAD;
    m->parentwrite = PARENTWRITE;*/

    mkfifo("ptc", 0755);
    mkfifo("ctp", 0755);
    status = fork();

    if (status == 0) {
      /*      dup2(CHILDREAD, 0);
      dup2(CHILDWRITE, 1);
      close(PARENTREAD);
      close(PARENTWRITE);*/
      system("perl test.pl");
    } else {
      /*      //    dup2(PARENTREAD, 0);
      //dup2(PARENTWRITE, 1);
      close(CHILDREAD);
      close(CHILDWRITE);
      */
      FILE *in = fopen("ptc", "r");
      m->ptc = in;

      FILE *out = fopen("ctp", "w");
      m->ctp = out;

      interactive_debug(m);

      fclose(in);
      fclose(out);
      unlink("ptc");
      unlink("ctp");
    }
  } else {
    interactive_debug(m);
  }
  
  return 0;
}
