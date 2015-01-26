// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "readline.h"

void rl_initialize(void) { }
void rl_bind_key(char c, void *f) { }
void *rl_insert; // unused

void add_history(char *line) { }

char *readline(char *prompt)
{
  printf("%s", prompt);

  int have = 0, capacity = 2;
  char *buf = NULL;

  do {
    capacity *= 2;
    char *buf2 = realloc(buf, capacity);
    if (!buf2) {
      free(buf);
      printf("out of memory\n");
      return NULL;
    }
    buf = buf2;

    if (!fgets(buf+have, capacity-have, stdin)) {
      free(buf);
      return NULL;
    }

    have += strlen(buf+have);

  } while (buf[have-1] != '\n');

  buf[have-1] = '\0';
  return buf;
}
