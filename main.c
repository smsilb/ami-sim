// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sim.h"

void usage(char *av0, char *msg, ...)
{
  va_list ap;
  if (msg) {
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1);
  }
  fprintf(stderr, 
      "usage: %s [options] <-program> [-args]\n"
      "options can be:\n"
      "  -d        Use interactive debugger\n"
      "  -k <key>  Randomize stack based on <key> instead of $NETID or $USER environment variable\n"
      "  -r        Randomize stack based on rand() instead of $NETID or $USER environment variable\n"
      "  -f <dir>  Set MIPS machine's filesystem root to <dir>; only files in <dir> can be opened by the MIPS program\n"
      "  -i <file> Use <file> as the program's standard input rather than the console (useful when using the debugger)\n"
      "  -o <file> Use <file> as the program's standard output rather than the console\n"
      "  -noargs   Do not pass any arguments to main on the stack\n",
      av0);
  exit(1);
}


int main(int ac, char **av)
{
  char *av0 = *(av++); ac--;

  struct ami_machine *m = create_ami_machine();

  m->filename = strdup(*av);
  m->reg_count = 1;

  printf("Filename: %s\n", m->filename);
  allocate_stack(m);

  interactive_debug(m);
  
  return 0;
}
