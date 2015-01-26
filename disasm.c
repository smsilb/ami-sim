// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: kwalsh@cs.holycross.edu

#include <stdio.h>

#include "sim.h"

char *RN[] = {
  "$0",
  "$at",
  "$v0", "$v1",
  "$a0", "$a1", "$a2", "$a3",
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$t8", "$t9",
  "$k0", "$k1",
  "$gp", "$sp", "$fp", "$ra"
};

void dump_registers(struct mips_machine *m) {
  printf(" ");
  for (int i = 0; i < 8; i++) {
    printf("%s = 0x%08x    %s = 0x%08x    %s = 0x%08x    %s = 0x%08x\n",
	RN[i], m->R[i], RN[i+8], m->R[i+8], RN[i+16], m->R[i+16], RN[i+24], m->R[i+24]); 
  }
  printf("$hi = 0x%08x    $pc = 0x%08x\n"
         "$lo = 0x%08x   $npc = 0x%08x\n",
      m->HI, m->PC, m->LO, m->nPC);
}


void dump_disassembly(FILE *out, unsigned int pc, unsigned int inst)
{
  // decode inst (not all fields are used for every inst)
  unsigned int op = inst >> 26;
  unsigned int rs = (inst >> 21) & 0x1f;
  unsigned int rt = (inst >> 16) & 0x1f;
  unsigned int rd = (inst >> 11) & 0x1f;
  unsigned int sa = (inst >> 6) & 0x1f;
  unsigned int offset = 0xffff & inst;
  unsigned int cop = (inst >> 26) - COP0;

  switch(op) {

    case SPECIAL:
      switch(inst & 0x3f) {
#define SHIFTC(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, %d\n", pc, inst, RN[rd], RN[rt], sa)
	case SLL: SHIFTC("sll"); break;
	case SRL: SHIFTC("srl"); break;
	case SRA: SHIFTC("sra"); break;
#define SHIFTV(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, %s\n", pc, inst, RN[rd], RN[rt], RN[rs])
	case SLLV: SHIFTV("sllv"); break;
	case SRLV: SHIFTV("srlv"); break;
	case SRAV: SHIFTV("srav"); break;
#define REGRS(op) fprintf(out, "0x%08x: %08x: " op " %s\n", pc, inst, RN[rs])
	case JR: REGRS("jr"); break;
	case JALR: fprintf(out, "0x%08x: %08x: " "jalr %s, %s\n", pc, inst, RN[rd], RN[rs]); break;
	case SYSCALL: fprintf(out, "0x%08x: %08x: " "syscall\n", pc, inst); break;
	case BREAK: fprintf(out, "0x%08x: %08x: " "break\n", pc, inst); break;
#define REGRD(op) fprintf(out, "0x%08x: %08x: " op " %s\n", pc, inst, RN[rd])
	case MFHI: REGRD("mfhi"); break;
	case MFLO: REGRD("mflo"); break;
	case MTHI: REGRS("mthi"); break;
	case MTLO: REGRS("mtlo"); break;
#define REG2(op) fprintf(out, "0x%08x: %08x: " op " %s, %s\n", pc, inst, RN[rs], RN[rt])
	case MULT: REG2("mult"); break;
	case MULTU: REG2("multu"); break;
	case DIV: REG2("div"); break;
	case DIVU: REG2("divu"); break;
#define REG3(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, %s\n", pc, inst, RN[rd], RN[rs], RN[rt])
	case ADD: REG3("add"); break;
	case ADDU: REG3("addu"); break;
	case SUB: REG3("sub"); break;
	case SUBU: REG3("subu"); break;
	case AND: REG3("and"); break;
	case OR: REG3("or"); break;
	case XOR: REG3("xor"); break;
	case NOR: REG3("nor"); break;
	case SLT: REG3("slt"); break;
	case SLTU: REG3("sltu"); break;
	default: fprintf(out, "0x%08x: %08x: " "<op=special,rs=%d,rt=%d,rd=%d,sa=%d,funct=%d>\n", pc, inst, rs, rt, rd, sa, inst & 0x3f); break;
      }
      break;

    case REGIMM:
      switch(rt) {
#define BRANCH1(op) fprintf(out, "0x%08x: %08x: " op " %s, 0x%08x\n", pc, inst, RN[rs], pc+4+(SIGNEX(offset) << 2))
	case BLTZ: BRANCH1("bltz"); break;
	case BLTZAL: BRANCH1("bltzal"); break;
	case BGEZ: BRANCH1("bgez"); break;
	case BGEZAL: BRANCH1("bgezal"); break;
	default: fprintf(out, "0x%08x: %08x: " "<op=regimm,rs=%d,rt=%d,imm=0x%04x>\n", pc, inst, rs, rt, offset); break;
      }
      break;

#define JUMP(op) fprintf(out, "0x%08x: %08x: " op " 0x%08x\n", pc, inst, ((pc + 4) & 0xf0000000) + ((inst & 0x03ffffff) << 2))
    case J: JUMP("j"); break;
    case JAL: JUMP("jal"); break;

#define BRANCH2(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, 0x%08x\n", pc, inst, RN[rs], RN[rt], pc+4+(SIGNEX(offset) << 2))
    case BEQ: BRANCH2("beq"); break;
    case BNE: BRANCH2("bne"); break;
    case BLEZ: BRANCH1("blez"); break;
    case BGTZ: BRANCH1("bgtz"); break;

#define REGIMM(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, %d\n", pc, inst, RN[rt], RN[rs], SIGNEX(offset))
#define REGIMMU(op) fprintf(out, "0x%08x: %08x: " op " %s, %s, %d\n", pc, inst, RN[rt], RN[rs], offset)
    case ADDI: REGIMM("addi"); break;
    case ADDIU: REGIMM("addiu"); break;
    case SLTI: REGIMM("slti"); break;
    case SLTIU: REGIMM("sltiu"); break;
    case ANDI: REGIMMU("andi"); break;
    case ORI: REGIMMU("ori"); break;
    case XORI: REGIMMU("xori"); break;
    case LUI: fprintf(out, "0x%08x: %08x: " "lui %s, 0x%0x\n", pc, inst, RN[rt], offset); break;

#define MEM(op) fprintf(out, "0x%08x: %08x: " op " %s, %d(%s)\n", pc, inst, RN[rt], SIGNEX(offset), RN[rs])
    case LB: MEM("lb"); break;
    case LH: MEM("lh"); break;
    case LW: MEM("lw"); break;
    case LBU: MEM("lbu"); break;
    case LHU: MEM("lhu"); break;
    case SB: MEM("sb"); break;
    case SH: MEM("sh"); break;
    case SW: MEM("sw"); break;
    case LWL: MEM("lwl"); break;
    case LWR: MEM("lwr"); break;
    case SWL: MEM("swl"); break;
    case SWR: MEM("swr"); break;

    case COP0:
    case COP1:
    case COP2:
    case COP3:
#define RTRD(op) fprintf(out, "0x%08x: %08x: " op "%d %s, %s\n", pc, inst, cop, RN[rt], RN[rd])
	      if(cop != 1 || rs == BC) { /* Not an FPU operation, or BC jump */
		switch(rs) {
		  case MF: RTRD("mfc"); break;
		  case CF: RTRD("cfc"); break;
		  case MT: RTRD("mtc"); break;
		  case CT: RTRD("ctc"); break;
		  case BC: fprintf(out, "0x%08x: %08x: " "bc%d%c 0x%08x", pc, inst, cop, (rt == BCF ? 'f' : 't'), pc+4+(SIGNEX(offset) << 2)); break;
		  default: fprintf(out, "0x%08x: %08x: " "<op=cop%d,rs=%d>\n", pc, inst, cop, rs); break;
		}
	      } else {
		int fmt = ((inst >> 21) & 0x1f) - 16; /* S=0, D=1, W=4 */
		int ft  = (inst >> 16) & 0x1f;
		int fs  = (inst >> 11) & 0x1f;
		int fd  = (inst >> 6) & 0x1f;
		switch(rs) {
#define RTFS(op) fprintf(out, "0x%08x: %08x: " op "%d %s, $F%d\n", pc, inst, cop, RN[rt], fs)
		  case MF: RTFS("mfc"); break;
		  case MT: RTFS("mtc"); break;
#define FRTRD(op) fprintf(out, "0x%08x: %08x: " op "%d %s, $F%d\n", pc, inst, cop, RN[rt], rd)
		  case CT: FRTRD("ctc"); break;
		  case CF: FRTRD("cfc"); break;
		  // case BC: /* see (cop != 1) case above */
		  case FF_S:
		  case FF_D:
		  case FF_W: /* FPU function */
			   switch(inst & 0x3f) {
#define FDFS(op) fprintf(out, "0x%08x: %08x: " op ".%c $F%d, $F%d\n", pc, inst, (fmt == S) ? 's' : (fmt == D ? 'd' : 'w'), fd, fs)
			     case FABS: FDFS("abs"); break;
			     case FNEG: FDFS("neg"); break;
			     case FMOV: FDFS("mov"); break;
#define FDFSFT(op) fprintf(out, "0x%08x: %08x: " op ".%c $F%d, $F%d, $F%d\n", pc, inst, (fmt == S) ? 's' : 'd', fd, fs, ft)
			     case FADD: FDFSFT("add"); break;
			     case FSUB:FDFSFT("sub"); break;
			     case FDIV:FDFSFT("div"); break;
			     case FMUL:FDFSFT("mul"); break;

#define FPCOMPARE(op) fprintf(out, "0x%08x: %08x: " op ".%c $F%d, $F%d\n", pc, inst, (fmt == S) ? 's' : 'd', fs, ft)
			     case C_F: FPCOMPARE("c.f"); break;
			     case CUN: FPCOMPARE("c.un"); break;
			     case CEQ: FPCOMPARE("c.eq"); break;
			     case CUEQ: FPCOMPARE("c.ueq"); break;
			     case COLT: FPCOMPARE("c.olt"); break;
			     case CULT: FPCOMPARE("c.ult"); break;
			     case COLE: FPCOMPARE("c.ole"); break;
			     case CULE: FPCOMPARE("c.ule"); break;
			     case CSF: FPCOMPARE("c.sf"); break;
			     case CNGLE: FPCOMPARE("c.ngle"); break;
			     case CSEQ: FPCOMPARE("c.seq"); break;
			     case CNGL: FPCOMPARE("c.ngl"); break;
			     case CLT:  FPCOMPARE("c.lt"); break;
			     case CNGE: FPCOMPARE("c.nge"); break;
			     case CLE: FPCOMPARE("c.le"); break;
			     case CNGT:  FPCOMPARE("c.ngt"); break;

			     case FCVTD: FDFS("cvt.d"); break;
			     case FCVTS: FDFS("cvt.s"); break;
			     case FCVTW: FDFS("cvt.w"); break;
			     default: fprintf(out, "0x%08x: %08x: " "<op=cop%d,rs=%d>\n", pc, inst, cop, rs); break;
			   }
			   break;
		  default: fprintf(out, "0x%08x: %08x: " "<op=cop%d,rs=%d>\n", pc, inst, cop, rs); break;
		}
	      }
	      break;

#define FMEM(op) fprintf(out, "0x%08x: %08x: " op " $F%d, %d(%s)\n", pc, inst, rt, SIGNEX(offset), RN[rs])
    case LWC0: MEM("lwc0"); break;
    case LWC1: FMEM("lwc1"); break;
    case LWC2: MEM("lwc2"); break;
    case LWC3: MEM("lwc3"); break;
    case SWC0: MEM("swc0"); break;
    case SWC1: FMEM("swc1"); break;
    case SWC2: MEM("swc2"); break;
    case SWC3: MEM("swc3"); break;

    default: fprintf(out, "0x%08x: %08x: " "<op=%d>\n", pc, inst, op); break;
  }

}

