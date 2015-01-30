// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sim.h"

unsigned short gethalf(void *p) { return *(unsigned short *)p; }
unsigned int getfull(void *p) { return *(unsigned int *)p; }

static void *readfile(char *filename, int *pfilesize)
{
  struct stat fileinfo;
  if (stat(filename, &fileinfo) < 0) {
    perror("Cannot open file"); exit(1);
  }

  int filesize = (int)fileinfo.st_size;
#ifdef READELF_DEBUG
  printf("File size is: %d\n", filesize);
#endif

  FILE *fd = fopen(filename, "r");
  if (!fd) {
    perror("Cannot open file"); exit(1);
  }

  void* buf = malloc(filesize);
  if (!buf) {
    perror("malloc failed"); exit(1);
  }

  if (fread(buf, 1, filesize, fd) <= 0) {
    perror("Cannot read file"); exit(1);
  }

  fclose(fd);

  *pfilesize = filesize;
  return buf;
}

