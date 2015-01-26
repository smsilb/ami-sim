// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <elf.h>

// #define READELF_DEBUG

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

void readelf(struct mips_machine *m, char* filename)
{
  int filesize = 0;
  void *buf;
  Elf32_Ehdr* ehdr = buf = readfile(filename, &filesize);

  if (filesize < sizeof(Elf32_Ehdr)) {
    fprintf(stderr, "file does not appear to be a proper executable (only %d bytes)\n", filesize);
    exit(1);
  }

  if (gethalf(&ehdr->e_machine) != EM_MIPS) {
    fprintf(stderr, "wrong architecture (0x%x); simulator can only handle little-endian MIPS (0x%x)\n", ehdr->e_machine, EM_MIPS_RS3_LE);
    exit(1);
  }

  if (gethalf(&ehdr->e_type) != ET_EXEC) {
    fprintf(stderr, "file type is 0x%x, not a proper executable\n", gethalf(&ehdr->e_type));
    exit(1);
  }

  m->PC = m->entry_point = getfull(&ehdr->e_entry);
  m->nPC = m->PC + 4;

#ifdef READELF_DEBUG
  printf("program headers: %d\n", gethalf(&ehdr->e_phnum));
  printf("program header offset: 0x%08x\n", getfull(&ehdr->e_phoff));
  printf("program header entry size: 0x%08x\n", gethalf(&ehdr->e_phentsize));
  printf("section headers: %d\n", gethalf(&ehdr->e_shnum));
  printf("section header offset: 0x%08x\n", getfull(&ehdr->e_shoff));
  printf("section header entry size: 0x%08x\n", gethalf(&ehdr->e_shentsize));
  printf("Entry point is: 0x%08x\n", m->entry_point);
#endif

  // Iterate over the section headers
  for (int i=0; i<gethalf(&ehdr->e_shnum); i++) {
    Elf32_Shdr* shdr;
    Elf32_Shdr* strtabhdr;
    char* section_name;
    int sh_offset, sh_size, sh_addr;

    shdr = (Elf32_Shdr*) (getfull(&ehdr->e_shoff) + gethalf(&ehdr->e_shentsize) * i + buf);
    strtabhdr = (Elf32_Shdr*) (getfull(&ehdr->e_shoff) + gethalf(&ehdr->e_shentsize)
                               * gethalf(&ehdr->e_shstrndx) + buf);
    section_name = buf + getfull(&strtabhdr->sh_offset) + getfull(&shdr->sh_name);

#ifdef READELF_DEBUG
    printf("in section[%d]: %s\n", i, section_name);
#endif

    if ( !(getfull(&shdr->sh_flags) & SHF_ALLOC) ) {
      continue;
    }


    sh_offset = getfull(&shdr->sh_offset);
    sh_size = getfull(&shdr->sh_size);
    sh_addr = getfull(&shdr->sh_addr);

    struct segment *seg = allocate_segment(m, sh_addr, sh_size, section_name);

#ifdef READELF_DEBUG
      printf("Section offset=0x%08x, size=0x%08x, addr=0x%08x\n",
             sh_offset, sh_size, sh_addr);
#endif

    if (! (getfull(&shdr->sh_type) & SHT_NOBITS )) {
      memcpy(seg->data, buf+sh_offset, sh_size);
    }

  }

  // Iterate over program headers
  for (int i=0; i<gethalf(&ehdr->e_phnum); i++) {
    Elf32_Phdr* phdr;

    phdr = (Elf32_Phdr*) (getfull(&ehdr->e_phoff) + gethalf(&ehdr->e_phentsize) * i + buf);

    if (getfull(&phdr->p_type) == 0x70000000) {
      int foffset = 0;
      foffset = getfull(&phdr->p_offset);
      m->R[REG_GP] = getfull(buf + foffset + getfull(&phdr->p_filesz) - 4);
#ifdef READELF_DEBUG
      printf("Setting gp value to 0x%x\n", m->R[REG_GP]);
#endif
    }

  }

  free(buf);

}
