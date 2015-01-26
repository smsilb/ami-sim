// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

// a simple readline alternative to avoid annoying dependencies

void rl_initialize(void);
void rl_bind_key(char c, void *f);
extern void *rl_insert;

void add_history(char *line);
char *readline(char *prompt);
