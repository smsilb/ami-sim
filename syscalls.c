// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim.h"

#define SYS_printi 1
#define SYS_prints 2
#define SYS_putc 3
#define SYS_getc 4
#define SYS_rand 6
#define SYS_getuser 7
#define SYS_readfile 8
#define SYS_writefile 9
#define SYS_exit 10

void getuser(struct mips_machine *m, unsigned int bufaddr, int len)
{
  char *netid = getenv("NETID");
  if (!netid) netid = getenv("USER");
  if (!netid) {
    fprintf(stderr, "MIPS simulator warning: you must set the $NETID or $USER environment variables\n");
    exit(1);
  }

  netid = strdup(netid);
  if (strlen(netid)+1 > len)
    netid[len-1] = '\0';
  mem_write(m, bufaddr, netid, strlen(netid)+1);
  free(netid);
}

int sane_filename(char *s)
{
  // only legal filenames: don't start with '.' and contain only 'a-zA-Z0-9_.-'
  if (!s || !*s || *s == '.') return 0;
  char c;
  while ((c = *(s++))) {
    if ('a' <= c && c <= 'z') continue;
    if ('a' <= c && c <= 'Z') continue;
    if ('0' <= c && c <= '9') continue;
    if (c == '_' || c == '.' || c == '-') continue;
    return 0;
  }
  return 1;
}

char *make_path(char *dir, char *filename)
{
  char *path = malloc(strlen(dir) + strlen(filename) + 2);
  sprintf(path, "%s/%s", dir, filename);
  return path;
}

int readfile(struct mips_machine *m, char *filename, unsigned int bufaddr, int len)
{
  if (!m->opt_dir) {
    fprintf(stderr, "warning: MIPS simulator only allows file access if given '-f <dir>' option\n");
    return -1;
  }
  if (!sane_filename(filename))
    return -1;
  if (len <= 0)
    return -1;
  char *buf = malloc(len);
  if (buf == NULL)
    return 1;
  char *path = make_path(m->opt_dir, filename);
  FILE *f = fopen(path, "r");
  free(path);
  if (!f) {
    free(buf);
    return -1;
  }
  int n = fread(buf, 1, len, f);
  fclose(f);
  mem_write(m, bufaddr, buf, n);
  free(buf);
  return n;
}

int writefile(struct mips_machine *m, char *filename, unsigned int bufaddr, int len)
{
  if (!m->opt_dir) {
    fprintf(stderr, "warning: MIPS simulator only allows file access if given '-f <dir>' option\n");
    return -1;
  }
  if (!sane_filename(filename))
    return -1;
  if (len <= 0)
    return -1;
  char *buf = malloc(len);
  if (buf == NULL)
    return 1;
  mem_read(m, bufaddr, buf, len);
  char *path = make_path(m->opt_dir, filename);
  FILE *f = fopen(path, "w");
  free(path);
  if (!f) {
    free(buf);
    return -1;
  }
  int n = fwrite(buf, 1, len, f);
  fclose(f);
  free(buf);
  return n;
}

int dosyscall(struct mips_machine *m)
{
  int res = 0;

  char *str = NULL;

  switch (m->R[REG_V0]) {

    case SYS_printi: fprintf(m->opt_output, "%d", m->R[REG_A0]); fflush(m->opt_output); break;

    case SYS_prints: fprintf(m->opt_output, "%s", str = mem_strdup(m, m->R[REG_A0])); fflush(m->opt_output); break;

    case SYS_putc: putc(m->R[REG_A0], m->opt_output); fflush(m->opt_output); break;

    case SYS_getc: res = getc(m->opt_input); break;

    case SYS_rand: res = rand(); break;

    case SYS_getuser: getuser(m, m->R[REG_A0], m->R[REG_A1]); break;

    case SYS_readfile: res = readfile(m, str = mem_strdup(m, m->R[REG_A0]), m->R[REG_A1], m->R[REG_A2]); break;

    case SYS_writefile: res = writefile(m, str = mem_strdup(m, m->R[REG_A0]), m->R[REG_A1], m->R[REG_A2]); break;

    case SYS_exit: return 1; break;

    default: res = -1; break;

  }

  if (str) free(str);

  m->R[REG_V0] = res;

  return 0;
}
