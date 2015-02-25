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
  char *av0 = *(av++); ac--;

  struct ami_machine *m = create_ami_machine();

  m->filename = strdup(*av);
  m->reg_count = 1;

  printf("Filename: %s\n", m->filename);
  allocate_stack(m);

  interactive_debug(m);
  
  return 0;
}
