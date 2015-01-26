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
      "usage: %s [options] <mips-program> [mips-args]\n"
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
  int opt_debug = 0, opt_noargs = 0, opt_count = 0, opt_argvsize = 0;

  struct mips_machine *m = create_mips_machine();

  while (ac > 0) {
    if (!strcmp("-h", *av) || !strcmp("-?", *av) || !strcmp("--help", *av)) {
      usage(av0, NULL);
    } else if (!strcmp("-noargs", *av)) {
      opt_noargs = 1;
    } else if (!strcmp("-d", *av)) {
      opt_debug = 1;
    } else if (!strcmp("-f", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-f' expects an argument\n");
      m->opt_dir = *av;
    } else if (!strcmp("-c", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-c' expects an argument\n");
      opt_count = atoi(*av);
    } else if (!strcmp("-v", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-v' expects an argument\n");
      opt_argvsize = atoi(*av);
    } else if (!strcmp("-i", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-i' expects an argument\n");
      m->opt_input = fopen(*av, "r");
      if (!m->opt_input) {
	perror(*av);
	usage(av0, "Can't open input file\n");
      }
    } else if (!strcmp("-o", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-o' expects an argument\n");
      m->opt_output = fopen(*av, "w");
      if (!m->opt_output) {
	perror(*av);
	usage(av0, "Can't open output file\n");
      }
    } else if (!strcmp("-k", *av)) {
      ac--; av++;
      if (!ac) usage(av0, "option '-k' expects an argument\n");
      m->opt_key = strdup(*av);
    } else if (!strcmp("-r", *av)) {
      m->opt_randkey = 1;
    } else if (**av == '-') {
      usage(av0, "unrecognized option '%s'\n", *av);
    } else {
      m->elf_filename = strdup(*av);
      break; // this and the remaining args are for the mips program
    }
    ac--; av++;
  }

  if (m->opt_randkey && m->opt_key)
    usage(av0, "Sorry, the -k and -r options are incompatible\n");

  if (!m->elf_filename) usage(av0, NULL);
  if (opt_noargs) {
    m->opt_ac = 1;
    m->opt_av = malloc(2 * sizeof(char *));
    if (opt_argvsize == 0)
      opt_argvsize = 6; // 6 chars to match the typical "server" arg for hw3
    m->opt_av[0] = calloc(opt_argvsize + 1, 1);
    memset(m->opt_av[0], '-', opt_argvsize);
    m->opt_av[1] = NULL;
  } else {
    m->opt_ac = ac;
    m->opt_av = av;
  }

  readelf(m, m->elf_filename);
  allocate_stack(m);
  push_arguments(m);

  if (opt_debug) {
    interactive_debug(m);
  } else {
    int err = setjmp(err_handler);
    if (err) {
      printf("MIPS processor has halted with status %d\n", err);
      return err;
    }

    err = run(m, opt_count);
    if (err == -RUN_EXIT) {
      show_exit_status(m);
      return m->R[REG_A0];
    } else if (err == -RUN_OK) {
      printf("MIPS processor has run for %d cycles, but not yet halted\n", opt_count);
      return 1;
    } else if (err == -RUN_BREAKPOINT){
      printf("MIPS processor has encountered a breakpoint, but debugging not enabled\n");
      return 1;
    } else {
      printf("MIPS processor has halted unexpectedly (err %d)\n", err);
      return 1;
    }
  }

  return 0;
}
