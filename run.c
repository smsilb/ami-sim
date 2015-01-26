// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

//#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "sim.h"

jmp_buf err_handler;

void raise(struct mips_machine *m, char *msg, ...)
{
  m->R[0] = 0; // insist that $zero stay zero
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);

  unsigned int *inst = _mem_ref(m, m->PC, 4);
  if (inst) {
    fprintf(stderr, "MIPS processor choked on instruction 0x%08x at pc 0x%08x\n", *inst, m->PC);
    fprintf(stderr, "Instruction was: ");
    dump_disassembly(stderr, m->PC, *inst);
    longjmp(err_handler, -RUN_FAULT);
  }
  else {
    fprintf(stderr, "MIPS processor choked at illegal pc 0x%08x\n", m->PC);
    longjmp(err_handler, -RUN_FAULT);
  }
  exit(1);
}


void long_multiply (struct mips_machine* m, unsigned int v1, unsigned int v2, int sign)
{
  unsigned long long prod;
  if (sign) // mult
    prod = ((signed long long)(int)v1) * ((signed long long)(int)v2);
  else // multu
    prod = ((unsigned long long)v1) * ((unsigned long long)v2);
  m->LO = (unsigned int)prod;
  m->HI = (unsigned int)(prod >> 32);
}

void long_divide (struct mips_machine* m, unsigned int v1, unsigned int v2, int sign)
{
  if (v2 == 0) raise(m, "DIVZERO: divide by zero error\n");
  if (sign) {
    m->LO = (signed)v1 / (signed)v2;
    m->HI = (signed)v1 % (signed)v2;
  } else {
    m->LO = v1 / v2;
    m->HI = v1 % v2;
  }
}

int _run(struct mips_machine* m, int count)
{
  for (;;) {

    m->R[0] = 0; // insist that $zero stay zero

    if (is_breakpoint(m, m->PC))
      return -RUN_BREAKPOINT;

    m->tsc++;

    int nnPC = m->nPC + 4;

    unsigned int inst = mem_read_word(m, m->PC);

    /*
    if (m->opt_dumpreg) dump_registers(m);
    if (m->opt_printstack) dump_stack(m);
    */
    if (m->opt_disassemble) dump_disassembly(stdout, m->PC, inst);

    // decode inst (not all fields are used for every inst)
    unsigned int op = inst >> 26;
    unsigned int rs = (inst >> 21) & 0x1f;
    unsigned int rt = (inst >> 16) & 0x1f;
    unsigned int rd = (inst >> 11) & 0x1f;
    unsigned int sa = (inst >> 6) & 0x1f;
    unsigned int offset = 0xffff & inst;
    unsigned int target = (m->nPC & 0xf0000000) + ((inst & 0x03ffffff) << 2);
    unsigned int cop = (inst >> 26) - COP0;

    switch(op) {

      case SPECIAL:
	switch(inst & 0x3f) {
	  case SLL:	m->R[rd] = m->R[rt] << sa; break;
	  case SRL:	m->R[rd] = m->R[rt] >> sa; break;
	  case SRA:	m->R[rd] = (((signed)m->R[rt]) >> sa); break;
	  case SLLV:	m->R[rd] = m->R[rt] << (m->R[rs] & 0x1f); break;
	  case SRLV:	m->R[rd] = m->R[rt] >> (m->R[rs] & 0x1f); break;
	  case SRAV:	m->R[rd] = ((signed)m->R[rt]) >> (m->R[rs] & 0x1f); break;
	  case JR:	nnPC = m->R[rs]; break;
	  case JALR:	m->R[rd] = m->nPC + 4; nnPC = m->R[rs]; break;
	  case SYSCALL: 
			if (dosyscall(m)) {
			  m->PC = 0;
			  m->nPC = 0;
			  return -RUN_EXIT;
			}
			break;
	  case BREAK:	
			m->PC = m->nPC;
			m->nPC = nnPC;
			return -RUN_BREAK;
	  case MFHI: 	m->R[rd] = m->HI; break;
	  case MTHI:	m->HI = m->R[rs]; break;
	  case MFLO: 	m->R[rd] = m->LO; break;
	  case MTLO:	m->LO = m->R[rs]; break;
	  case MULT:	long_multiply(m, m->R[rs], m->R[rt], 1); break;
	  case MULTU:	long_multiply(m, m->R[rs], m->R[rt], 0); break;
	  case DIV:	long_divide(m, m->R[rs], m->R[rt], 1); break;
	  case DIVU:	long_divide(m, m->R[rs], m->R[rt], 0); break;
	  case ADD:	m->R[rd] = (signed) m->R[rs] + (signed) m->R[rt];
			if ((NEG(m->R[rd]) && !NEG(m->R[rs]) && !NEG(m->R[rt])) ||
			    (!NEG(m->R[rd]) && NEG(m->R[rs]) && NEG(m->R[rt])))
			  raise(m, "OVERFLOW\n");
			break;
	  case ADDU:	m->R[rd] = m->R[rs] + m->R[rt]; break;
	  case SUB:	m->R[rd] = (signed) m->R[rs] - (signed) m->R[rt];
			if ((NEG(m->R[rd]) && !NEG(m->R[rs]) && NEG(m->R[rt])) ||
			    (!NEG(m->R[rd]) && NEG(m->R[rs]) && !NEG(m->R[rt])))
			  raise(m, "OVERFLOW\n");
			break;
	  case SUBU:	m->R[rd] = m->R[rs] - m->R[rt]; break;
	  case AND:	m->R[rd] = m->R[rs] & m->R[rt]; break;
	  case OR:	m->R[rd] = m->R[rs] | m->R[rt]; break;
	  case XOR:	m->R[rd] = m->R[rs] ^ m->R[rt]; break;
	  case NOR:	m->R[rd] = ~(m->R[rs] | m->R[rt]); break;
	  case SLT:	m->R[rd] = ((signed) m->R[rs] < (signed) m->R[rt]) ? 1: 0; break;
	  case SLTU:	m->R[rd] = (m->R[rs] < m->R[rt]) ? 1: 0; break;
	  default:	raise(m, "Unimplemented SPECIAL instruction\n");
	}
	break;

      case REGIMM:
	switch(rt) {
	  case BLTZ:
	    if ((signed) m->R[rs] < 0) nnPC = m->nPC + (SIGNEX(offset) << 2);
	    break;
	  case BLTZAL:
	    if ((signed) m->R[rs] < 0) {
	      m->R[31] = m->nPC + 4;
	      nnPC = m->nPC + (SIGNEX(offset) << 2);
	    }
	    break;
	  case BGEZ:
	    if ((signed) m->R[rs] >= 0)
	      nnPC = m->nPC + (SIGNEX(offset) << 2);
	    break;
	  case BGEZAL:
	    if ((signed) m->R[rs] >= 0) {
	      m->R[31] = m->nPC + 4;
	      nnPC = m->nPC + (SIGNEX(offset) << 2);
	    }
	    break;
	  default:
	    raise(m, "FAULT: illegal instruction\n");
	}
	break;

      case J:
	nnPC = target;
	break;
      case JAL:
	m->R[31] = m->nPC + 4;
	nnPC = target;
	break;
      case BEQ:
	if(m->R[rs] == m->R[rt])
	  nnPC = m->nPC + (SIGNEX(offset) << 2);
	break;
      case BNE:
	if(m->R[rs] != m->R[rt])
	  nnPC = m->nPC + (SIGNEX(offset) << 2);
	break;
      case BLEZ:
	if((signed) m->R[rs] <= 0)
	  nnPC = m->nPC + (SIGNEX(offset) << 2);
	break;
      case BGTZ:
	if((signed) m->R[rs] > 0)
	  nnPC = m->nPC + (SIGNEX(offset) << 2);
	break;
      case ADDI:
	if ((NEG(m->R[rt]) && !NEG(m->R[rs]) && !NEG(SIGNEX(offset))) ||
	    (!NEG(m->R[rt]) && NEG(m->R[rs]) && NEG(SIGNEX(offset))))
	  raise(m, "OVERFLOW\n");
	m->R[rt] = (signed) m->R[rs]+(signed)SIGNEX(offset);
	break;
      case ADDIU:   m->R[rt] = m->R[rs] + SIGNEX(offset); break;
      case SLTI:    m->R[rt] = ((signed) m->R[rs] < (signed) SIGNEX(offset)) ? 1 : 0; break;
      case SLTIU:   m->R[rt] = (m->R[rs] < SIGNEX(offset)) ? 1 : 0; break;
      case ANDI:    m->R[rt] = m->R[rs] & offset; break;
      case ORI:     m->R[rt] = m->R[rs] | offset; break;
      case XORI:    m->R[rt] = m->R[rs] ^ offset; break;
      case LUI:     m->R[rt] = (offset << 16); break;
      case LB:	    m->R[rt] = (signed int) ((signed char) mem_read_byte(m, m->R[rs] + SIGNEX(offset))); break;
      case LH:	    m->R[rt] = (signed int) ((signed short) mem_read_half(m, m->R[rs] + SIGNEX(offset))); break;
      case LW:	    m->R[rt] = (signed) mem_read_word(m, m->R[rs] + SIGNEX(offset)); break;
      case LBU:	    m->R[rt] = mem_read_byte(m, m->R[rs] + SIGNEX(offset)); break;
      case LHU:	    m->R[rt] = mem_read_half(m, m->R[rs] + SIGNEX(offset)); break;
      case SB:	    mem_write_byte(m, m->R[rs] + SIGNEX(offset), m->R[rt]); break;
      case SH:	    mem_write_half(m, m->R[rs] + SIGNEX(offset), m->R[rt]); break;
      case SW:	    mem_write_word(m, m->R[rs] + SIGNEX(offset), m->R[rt]); break;
      case LWL: {
		  // To load word at address 2, i.e. bytes 5, 4, 3, 2
		  // LWL 5 # loads 5, 4, _, _
		  // LWR 2 # loads _, _, 3, 2
		  unsigned int addr = m->R[rs] + SIGNEX(offset);
		  // addr points to MSB
		  // if addr == 7, load bytes 7, 6, 5, 4
		  // if addr == 6, load bytes 6, 5, 4, _
		  // if addr == 5, load bytes 5, 4, _, _
		  // if addr == 4, load bytes 4, _, _, _
		  unsigned int word = mem_read_word(m, addr & ~3);
		  switch(addr & 0x3) {
		    case 3: m->R[rt] = word; break;
		    case 2: m->R[rt] = (word <<  8) | (m->R[rt] & 0xff); break;
		    case 1: m->R[rt] = (word << 16) | (m->R[rt] & 0xffff); break;
		    case 0: m->R[rt] = (word << 24) | (m->R[rt] & 0xffffff); break;
		  }
		}
		break;
      case LWR: {
		  unsigned int addr = m->R[rs] + SIGNEX(offset);
		  // addr points to LSB
		  // if addr = 4, load bytes 7, 6, 5, 4
		  // if addr = 5, load bytes _, 7, 6, 5
		  // if addr = 6, load bytes _, _, 7, 6
		  // if addr = 7, load bytes _, _, _, 7
		  unsigned int word = mem_read_word(m, addr & ~3);
		  switch(addr & 0x3) {
		    case 0: m->R[rt] = word; break;
		    case 1: m->R[rt] = (m->R[rt] & 0xff000000) | (word >> 8); break;
		    case 2: m->R[rt] = (m->R[rt] & 0xffff0000) | (word >> 16); break;
		    case 3: m->R[rt] = (m->R[rt] & 0xffffff00) | (word >> 24); break;
		  }
		}
		break;
      case SWL: {
		  unsigned int addr = m->R[rs] + SIGNEX(offset);
		  // addr points to MSB
		  // if addr == 7, store bytes A, B, C, D
		  // if addr == 6, store bytes _, A, B, C
		  // if addr == 5, store bytes _, _, A, B
		  // if addr == 4, store bytes _, _, _, A
		  unsigned int data = mem_read_word(m, addr & ~3);
		  switch(addr & 0x3) {
		    case 3: data = m->R[rt]; break;
		    case 2: data = (data & 0xff000000) | (m->R[rt] >> 8); break;
		    case 1: data = (data & 0xffff0000) | (m->R[rt] >> 16); break;
		    case 0: data = (data & 0xffffff00) | (m->R[rt] >> 24); break;
		  }
		  mem_write_word(m, addr & ~3, data);
		}
		break;
      case SWR: {
		  unsigned int addr = m->R[rs] + SIGNEX(offset);
		  // addr points to LSB
		  // if addr = 4, store bytes A, B, C, D
		  // if addr = 5, store bytes B, C, D, _
		  // if addr = 6, store bytes C, D, _, _
		  // if addr = 7, store bytes D, _, _, _
		  unsigned int data = mem_read_word(m, addr & ~3);
		  switch(addr & 0x3) {
		    case 0: data = m->R[rt]; break;
		    case 1: data = (m->R[rt] <<  8) | (data & 0xff); break;
		    case 2: data = (m->R[rt] << 16) | (data & 0xffff); break;
		    case 3: data = (m->R[rt] << 24) | (data & 0xffffff); break;
		  }
		  mem_write_word(m, addr & ~3, data);
		}
		break;
      case COP0:
      case COP1:
      case COP2:
      case COP3:
		if(cop != 1) { /* Not an FPU operation */
		  switch(rs) {
		    case MF: m->R[rt] = m->CPR[cop][rd]; break;
		    case CF: m->R[rt] = m->CCR[cop][rd]; break;
		    case MT: m->CPR[cop][rd] = m->R[rt]; break;
		    case CT: m->CCR[cop][rd] = m->R[rt]; break;
		    case BC:
			     if(rt == BCF) {
			       if(m->CpCond[cop] == 0) nnPC = m->nPC + (SIGNEX(offset) << 2);
			     } else if (rt == BCT) {
			       if(m->CpCond[cop] != 0) nnPC = m->nPC + (SIGNEX(offset) << 2);
			     } else {
			       raise(m, "Coprocessor invalid rt field in BC\n");
			     }
			     break;
		    case CO: raise(m, "FAULT: unknown coprocessor instruction\n");
		    default: raise(m, "FAULT: unknown coprocessor operation\n");
		  }
		} else {
		  int fmt = ((inst >> 21) & 0x1f) - 16; /* S=0, D=1, W=4 */
		  int ft  = (inst >> 16) & 0x1f;
		  int fs  = (inst >> 11) & 0x1f;
		  int fd  = (inst >> 6) & 0x1f;
		  switch(rs) {
		    case MF: m->R[rt] = *(unsigned int *)&m->FGR[fs]; break;
		    case MT: m->FGR[fs] = *(float *)&m->R[rt]; break;
		    case CT: m->CCR[1][rd] = m->R[rt]; break;
		    case CF: m->R[rt] = m->CCR[1][rd]; break;
		    case BC:
			     if(((rt == BCF) && (FpCond == 0)) || ((rt == BCT) && (FpCond != 0)))
			       nnPC = m->nPC + (SIGNEX(offset) << 2);
			     break;
		    case FF_S:
		    case FF_D:
		    case FF_W: /* FPU function */
			     {
			       double fsop, ftop, fpres = 0.0;
			       switch(fmt) {
				 case S: fsop = m->FGR[fs]; ftop = m->FGR[ft]; break;
				 case D: fsop = m->FPR[fs >> 1]; ftop = m->FPR[ft >> 1]; break;
				 case W: fsop = m->FWR[fs]; ftop = m->FWR[ft]; break;
				 default: raise(m, "FAULT: unknown floating point format\n");
			       }
			       switch(inst & 0x3f) {
				 case FABS: fpres = fabs(fsop); break;
				 case FADD: fpres = fsop + ftop; break;
				 case FSUB: fpres = fsop - ftop; break;
				 case FDIV:
					    if (ftop==0) raise(m, "FDIVZERO: floating point divide by zero error\n");
					    fpres = fsop / ftop;
					    break;
				 case FMUL: fpres = fsop * ftop; break;
				 case FNEG: fpres = -fsop; break;
				 case FMOV: fpres = fsop; break;
				 case FCVTD: fmt = D; fpres = fsop; break;
				 case FCVTS: fmt = S; fpres = fsop; break;
				 case FCVTW: fmt = W; fpres = fsop; break;
				 case C_F:
				 case CUN:
				 case CEQ:
				 case CUEQ:
				 case COLT:
				 case CULT:
				 case COLE:
				 case CULE:
				 case CSF:
				 case CNGLE:
				 case CSEQ:
				 case CNGL:
				 case CLT:
				 case CNGE:
				 case CLE:
				 case CNGT: {
					      /* BUG BUG what happened to COND_IN ?? */
					      int less, equal, unordered;
					      int cond = inst & 0x1f;
					      if((fsop==NAN) || (ftop==NAN)) {
						less = 0;
						equal = 0;
						unordered = 1;
					      } else {
						less = fsop < ftop;
						equal = fsop == ftop;
						unordered = 0;
					      }
					      FpCond = ((COND_LT&cond) ? less : 0) |
						((COND_EQ&cond) ? equal: 0) |
						((COND_UN&cond) ? unordered: 0);
					      goto fp_noupdate; /* don't fall out to the following switch */
					    }
					    break;
				 default: raise(m, "FAULT: unknown floating point function\n");
			       }
			       switch(fmt) {
				 case S: m->FGR[fd] = (float) fpres; break;
				 case D: m->FPR[fd >> 1] = (double) fpres; break;
				 case W: m->FWR[fd] = (int) fpres; break;
					 /* no need for a default because they are caught earlier */
			       }
fp_noupdate:
			       ;
			     }
			     break;
		    default: raise(m, "FAULT: unknown rs field of coprocessor instruction\n");
		  }
		}
		break;
      case LWC1: *(unsigned int *)&m->FGR[rt] = mem_read_word(m, m->R[rs]+SIGNEX(offset)); break;
      case SWC1: mem_write_word(m, m->R[rs] + SIGNEX(offset), *(unsigned int *)&m->FGR[rt]); break;
      case LWC0:
      case LWC2:
      case LWC3:
		 m->CPR[(inst >> 26)-LWC0][rt] = mem_read_word(m, m->R[rs]+SIGNEX(offset));
		 break;
      case SWC0:
      case SWC2:
      case SWC3:
		 mem_write_word(m, m->R[rs] + SIGNEX(offset), m->CPR[(inst>>26) - SWC0][rt]);
		 break;
      default:
		 raise(m, "FAULT: unknown opcode field of instruction\n");
    }

    m->PC = m->nPC;
    m->nPC = nnPC;

    if (count > 0 && --count == 0)
      return -RUN_OK;
  }

}


int run(struct mips_machine* m, int count)
{
  struct timespec tstart, tend;
  int err1 = clock_gettime(CLOCK_MONOTONIC, &tstart);
  int ret = _run(m, count);
  int err2 = clock_gettime(CLOCK_MONOTONIC, &tend);

  if (err1 || err2) {
    perror("clock_gettime");
  }
  if (tend.tv_nsec < tstart.tv_nsec) {
    tend.tv_nsec -= (tstart.tv_nsec - 1000000000);
    tend.tv_sec -= (tstart.tv_sec + 1);
  } else {
    tend.tv_nsec -= (tstart.tv_nsec);
    tend.tv_sec -= (tstart.tv_sec);
  }
  long long nsec = 1000000000 * (long long)tend.tv_sec + (long long)tend.tv_nsec;
  m->elapsed += nsec;

  return ret;
}

void show_exit_status(struct mips_machine *m)
{
  if (!m->elapsed) {
    printf("MIPS program exits with status %d (approx. %lld instructions at ?? Hz) \n", m->R[REG_A0], m->tsc);
  } else {
    long long hz = m->tsc * 1000000000 / m->elapsed;
    long long khz = hz/1000;
    long long mhz = khz/1000;
    long long ghz = mhz/1000;
    if (ghz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld GHz)\n", m->R[REG_A0], m->tsc, m->elapsed, ghz, mhz);
    else if (mhz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld MHz)\n", m->R[REG_A0], m->tsc, m->elapsed, mhz, khz);
    else if (khz > 0)
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld.%03lld KHz)\n", m->R[REG_A0], m->tsc, m->elapsed, khz, hz);
    else 
      printf("MIPS program exits with status %d (approx. %lld instructions in %lld nsec at %lld Hz)\n", m->R[REG_A0], m->tsc, m->elapsed, hz);
  }
}

