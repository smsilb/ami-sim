// Copyright (c) 2015, Sam Silberstein.  All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License").
// Author: smsilb14@g.holycross.edu

//#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "sim.h"

jmp_buf err_handler;

void raise(struct ami_machine *m, char *msg)
{
    char *inst = m->mem[m->PC].instruction;
    if (inst) {
        printf("AMI processor choked on instruction %s with message: %s\n", inst, msg);
    } else {
        printf("AMI processor choked at illegal pc %d with message: %s\n", m->PC, msg);
    }

    if (m->opt_graphical) {
        *(m->shm + 2) = '\0';
        *(m->shm + 1) = 'q';
        *(m->shm) = 'r';
    }
    exit(1);
}

int _run(struct ami_machine* m, int count)
{
    int op, addr1, addr2;
    char str[20];
    struct stack_entry entry;
    for (;;) {
        if (m->halted)
            return -RUN_HALTED;

        if (is_breakpoint(m, m->PC)) {
            return -RUN_BREAKPOINT;
        }

        m->nPC = m->PC + 1;

        op = m->mem[m->PC].op;
        entry = m->mem[m->PC];

        if (!m->opt_graphical) {
            printf("%s\n", entry.instruction);
        }

        switch(op) {
        case HALT:
            if (!m->opt_graphical) {
                printf("HALT\n");
            }
            m->halted = 1;
            break;
        case WRITE:
            if (m->opt_graphical) {
                m->console_io_value  = arg_get_value(m, entry.arguments[0]);
                m->console_io_status = 2;
            } else {
                printf("WRITE -> %i\n", arg_get_value(m, entry.arguments[0]));
            }
            break;
        case READB:
            if (entry.argc == 1) {
                addr1 = mem_get_addr(m, entry.arguments[0]);
                if (m->opt_graphical) {
                    //send input prompt
                    //wait for input
                    m->console_io_status = 1;
                    update_gui(m);
                    mem_write(m, addr1, m->console_io_value != 0);
                } else {
                    fgets(str, 20, stdin);
                    mem_write(m, addr1, atoi(str) != 0);
                    printf("READB, mem[%i] <- %i\n", addr1, atoi(str) != 0);
                }
            } else {
                raise(m, "Non address destination for READB");
            }
            break;
        case READI:
            if (entry.argc == 1) {
                addr1 = mem_get_addr(m, entry.arguments[0]);
                if (m->opt_graphical) {
                    //send input prompt
                    //wait for input
                    m->console_io_status = 1;
                    update_gui(m);
                    mem_write(m, addr1, m->console_io_value);
                } else {
                    fgets(str, 20, stdin);
                    mem_write(m, addr1, atoi(str));
                    printf("READI, mem[%i] <- %i\n", addr1, m->mem[addr1].data);
                }
            } else {
                raise(m, "Non address destination for READI");
            }
            break;
        case JUMP:
            addr1 = add_get_value(m, entry.arguments[0]);
            if (addr1 < m->slots_used) {
                m->nPC = addr1;
                if (!m->opt_graphical) {
                    printf("JUMP to %i\n", addr1);
                }
            } else {
                raise(m, "Attempted to jump past instructions in stack");
            }
            break;
        case JUMPIF:
            addr1 = add_get_value(m, entry.arguments[0]);
            if (arg_get_value(m, entry.arguments[1])) {
                m->nPC = addr1;
                if (!m->opt_graphical) {
                    printf("JUMPIF to %i, COND TRUE\n", addr1);
                }
            } else {
                if (!m->opt_graphical) {
                    printf("JUMPIF to %i, COND FALSE\n", addr1);
                }
            }
            break;
        case JUMPNIF:
            addr1 = add_get_value(m, entry.arguments[0]);
            if (arg_get_value(m, entry.arguments[1]) == 0) {
                m->nPC = addr1;
                if (!m->opt_graphical) {
                    printf("JUMPNIF to %i, COND TRUE\n", addr1);
                }
            } else {
                if (!m->opt_graphical) {
                printf("JUMPNIF to %i, COND FALSE\n", addr1);
                }
            }
            break;
        case MOVE:
            if (entry.arguments[0].type == REGISTER) {
                m->R[entry.arguments[0].reg] = arg_get_value(m, entry.arguments[1]);
                if (!m->opt_graphical) {
                    printf("MOVE, r%i <- %i\n", 
                           entry.arguments[0].reg, arg_get_value(m, entry.arguments[1]));
                }
            } else if (entry.arguments[0].type == ADDRESS) {
                addr1 = mem_get_addr(m, entry.arguments[0]);
                mem_write(m, addr1,  arg_get_value(m, entry.arguments[1]));
                if (!m->opt_graphical) {
                    printf("MOVE, mem[%i] <- %i\n",
                           addr1, arg_get_value(m, entry.arguments[1]));
                }
            } else {
                raise(m, "Inappropriate destination for move");
            }
            break;
        case LOAD:
            if (entry.argc == 2) {
                addr1 = mem_get_addr(m, entry.arguments[1]);
                m->R[entry.arguments[0].reg] = mem_read(m, addr1);
                if (!m->opt_graphical) {
                    printf("LOAD, r%i <- %i\n", 
                           entry.arguments[0].reg, mem_read(m, addr1));
                }
            } else {
                raise(m, "Inappropriate destination for load");
            }
            break;
        case STORE:
            if (entry.argc == 2) {
                addr1 = mem_get_addr(m, entry.arguments[0]);
                mem_write(m, addr1, m->R[entry.arguments[1].reg]);
                if (!m->opt_graphical) {
                    printf("STORE, mem[%i] <- %i\n", 
                           addr1, m->R[entry.arguments[1].reg]);
                }
            } else {
                raise(m, "Inappropriate destination for store");
            }
            break;
        case IDM:
            if (entry.arguments[1].type == NUMBER) {
                if (entry.arguments[0].type == REGISTER) {
                    m->R[entry.arguments[0].reg] = entry.arguments[1].number;
                    if (!m->opt_graphical) {
                        printf("IDM, r%i <- %i\n", entry.arguments[0].reg, entry.arguments[1].number);
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    mem_write(m, addr1, entry.arguments[1].number);
                    if (!m->opt_graphical) {
                        printf("IDM, mem[%i] <- %i\n", addr1, entry.arguments[1].number);
                    }
                } else {
                    raise(m, "Inappropriate destination for immediate data move");
                }
            } else {
                if (!m->opt_graphical) {
                    printf("%i\n", entry.arguments[0].type);
                }
                raise(m, "Inappropriate number for immediate data move");
            }

            break;
        case EQ:
            if (entry.argc == 3) {
                addr1 = entry.arguments[0].reg;
                if (arg_get_value(m, entry.arguments[1]) ==
                    arg_get_value(m, entry.arguments[2])) {
                    m->R[addr1] = 1;
                    if (!m->opt_graphical) {
                        printf("EQ, r%i <- true\n", addr1);
                    }
                } else {
                    m->R[addr1] = 0;
                    if (!m->opt_graphical) {
                        printf("EQ, r%i <- false\n", addr1);
                    }
                }
            } else {
                raise(m, "Non register argument in EQ instruction");
            } 
            break;
        case NEQ:
            if (entry.argc == 3) {
                addr1 = entry.arguments[0].reg;
                if (arg_get_value(m, entry.arguments[1]) ==
                    arg_get_value(m, entry.arguments[2])) {
                    m->R[addr1] = 1;
                    if (!m->opt_graphical) {
                        printf("NEQ, r%i <- false\n", addr1);
                    }
                } else {
                    m->R[addr1] = 0;
                    if (!m->opt_graphical) {
                        printf("NEQ, r%i <- true\n", addr1);
                    }
                }
            } else {
                raise(m, "Non register argument in NEQ instruction");
            }
            break;
        case LT:
            if (entry.argc == 3) {
                addr1 = entry.arguments[0].reg;
                if (arg_get_value(m, entry.arguments[1]) <
                    arg_get_value(m, entry.arguments[2])) {
                    m->R[addr1] = 1;
                    if (!m->opt_graphical) {
                        printf("LT, r%i <- true\n", addr1);
                    }
                } else {
                    m->R[addr1] = 0;
                    if (!m->opt_graphical) {
                        printf("LT, r%i <- false\n", addr1);
                    }
                }
            } else {
                raise(m, "Non register argument in LT instruction");
            }
            break;
        case LTE:
            if (entry.argc == 3) {
                addr1 = entry.arguments[0].reg;
                if (arg_get_value(m, entry.arguments[1]) <=
                    arg_get_value(m, entry.arguments[2])) {
                    m->R[addr1] = 1;
                    if (!m->opt_graphical) {
                        printf("LTE, r%i <- true\n", addr1);
                    }
                } else {
                    m->R[addr1] = 0;
                    if (!m->opt_graphical) {
                        printf("LTE, r%i <- false\n", addr1);
                    }
                }
            } else {
                raise(m, "Non register argument in LTE instruction");
            }
            break;
        case AND:
            if (entry.argc == 3){
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    if (arg_get_value(m, entry.arguments[1]) != 0
                        && arg_get_value(m, entry.arguments[2]) != 0) {
                        m->R[addr1] = 1;
                        if (!m->opt_graphical) {
                            printf("AND, r%i <- true\n", addr1);
                        }
                    } else {
                        m->R[addr1] = 0;
                        if (!m->opt_graphical) {
                            printf("AND, r%i <- false\n", addr1);
                        }
                    } 
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    if (arg_get_value(m, entry.arguments[1]) != 0
                        && arg_get_value(m, entry.arguments[2]) != 0) {
                        mem_write(m, addr1, 1);
                        if (!m->opt_graphical) {
                            printf("AND, mem[%i] <- true\n", addr1);
                        }
                    } else {
                        mem_write(m, addr1, 0);
                        if (!m->opt_graphical) {
                            printf("AND, mem[%i] <- false\n", addr1);
                        }
                    } 
                } else {
                    raise(m, "Inappropriate destination for AND");
                }
            } else { 
                raise(m, "Wrong number of arguments for AND");
            }
            break;
        case OR:
            if (entry.argc == 3){
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    if (arg_get_value(m, entry.arguments[1]) != 0
                        || arg_get_value(m, entry.arguments[2]) != 0) {
                        m->R[addr1] = 1;
                        if (!m->opt_graphical) {
                            printf("OR, r%i <- true\n", addr1);
                        }
                    } else {
                        m->R[addr1] = 0;
                        if (!m->opt_graphical) {
                            printf("OR, r%i <- false\n", addr1);
                        }
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    if (arg_get_value(m, entry.arguments[1]) != 0
                        || arg_get_value(m, entry.arguments[2]) != 0) {
                        mem_write(m, addr1, 1);
                        if (!m->opt_graphical) {
                            printf("OR, mem[%i] <- true\n", addr1);
                        }
                    } else {
                        mem_write(m, addr1, 0);
                        if (!m->opt_graphical) {
                            printf("OR, mem[%i] <- false\n", addr1);
                        }
                    }
                } else {
                    raise(m, "Inappropriate destination for OR");
                }
            } else { 
                raise(m, "Wrong number of arguments for OR");
            }
            break;
        case NOT:
            if (entry.argc == 2) {
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    if (arg_get_value(m, entry.arguments[1]) == 0) {
                        m->R[addr1] = 1;
                        if (!m->opt_graphical) {
                            printf("NOT, r%i <- true\n", addr1);
                        }
                    } else {
                        m->R[addr1] = 0;
                        if (!m->opt_graphical) {
                            printf("NOT, r%i <- false\n", addr1);
                        }
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    if (arg_get_value(m, entry.arguments[1]) == 0) {
                        mem_write(m, addr1, 1);
                        if (!m->opt_graphical) {
                            printf("NOT, mem[%i] <- true\n", addr1);
                        }
                    } else {
                        mem_write(m, addr1, 0);
                        if (!m->opt_graphical) {
                            printf("NOT, mem[%i] <- false\n", addr1);
                        }
                    }
                } else {
                    raise(m, "Inappropriate destination for NOT");
                }
            } else { 
                raise(m, "Wrong number of arguments for NOT");
            }
            break;
        case ADD:
            if (entry.argc == 3) {
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
                        + arg_get_value(m, entry.arguments[2]);
                    if (!m->opt_graphical) {
                        printf("ADD, r%i <- %i\n", addr1, m->R[addr1]);
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    mem_write(m, addr1, arg_get_value(m, entry.arguments[1]) 
                              + arg_get_value(m, entry.arguments[2]));
                    if (!m->opt_graphical) {
                        printf("ADD, mem[%i] <- %i\n", addr1, mem_read(m, addr1));
                    }
                } else {
                    raise(m, "Inappropriate destination for ADD");
                }
            } else { 
                raise(m, "Wrong number of arguments for ADD");
            }
            break;
        case SUB:
            if (entry.argc == 3) {
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
                        - arg_get_value(m, entry.arguments[2]);
                    if (!m->opt_graphical) {
                        printf("SUB, r%i <- %i\n", addr1, m->R[addr1]);
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    mem_write(m, addr1, arg_get_value(m, entry.arguments[1]) 
                              - arg_get_value(m, entry.arguments[2]));
                    if (!m->opt_graphical) {
                        printf("SUB, mem[%i] <- %i\n", addr1, mem_read(m, addr1));
                    }
                } else {
                    raise(m, "Inappropriate destination for SUB");
                }
            } else { 
                raise(m, "Wrong number of arguments for SUB");
            }
            break;
        case MULT:
            if (entry.argc == 3) {
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
                        * arg_get_value(m, entry.arguments[2]);
                    if (!m->opt_graphical) {
                        printf("MULT, r%i <- %i\n", addr1, m->R[addr1]);
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    mem_write(m, addr1, arg_get_value(m, entry.arguments[1]) 
                              * arg_get_value(m, entry.arguments[2]));
                    if (!m->opt_graphical) {
                        printf("MULT, mem[%i] <- %i\n", addr1, mem_read(m, addr1));
                    }
                } else {
                    raise(m, "Inappropriate destination for MULT");
                }
            } else { 
                raise(m, "Wrong number of arguments for MULT");
            }
            break;
        case DIV:
            if (entry.argc == 3) {
                if (arg_get_value(m, entry.arguments[2]) == 0) {
                    raise(m, "Division by zero");
                } else {
                    if (entry.arguments[0].type == REGISTER) {
                        addr1 = entry.arguments[0].reg;
	
                        m->R[addr1] = arg_get_value(m, entry.arguments[1]) 
                            / arg_get_value(m, entry.arguments[2]);
                        if (!m->opt_graphical) {
                            printf("DIV, r%i <- %i\n", addr1, m->R[addr1]);
                        }
                    } else if (entry.arguments[0].type == ADDRESS) {
                        addr1 = mem_get_addr(m, entry.arguments[0]);
	
                        mem_write(m, addr1, arg_get_value(m, entry.arguments[1]) 
                                  / arg_get_value(m, entry.arguments[2]));
                        if (!m->opt_graphical) {
                            printf("DIV, mem[%i] <- %i\n", addr1, mem_read(m, addr1));
                        }
                    } else {
                        raise(m, "Inappropriate destination for DIV");
                    }
                }
            } else { 
                raise(m, "Wrong number of arguments for DIV");
            }
            break;
        case NEG:
            if (entry.argc == 2) {
                if (entry.arguments[0].type == REGISTER) {
                    addr1 = entry.arguments[0].reg;
                    m->R[addr1] = -1 * arg_get_value(m, entry.arguments[1]);
                    if (!m->opt_graphical) {
                        printf("NEG, r%i <- %i\n", addr1, m->R[addr1]);
                    }
                } else if (entry.arguments[0].type == ADDRESS) {
                    addr1 = mem_get_addr(m, entry.arguments[0]);
                    mem_write(m, addr1, -1 * arg_get_value(m, entry.arguments[1]));
                    if (!m->opt_graphical) {
                        printf("NEG, mem[%i] <- %i\n", addr1, mem_read(m, addr1));
                    }
                } else {
                    raise(m, "Inappropriate destination for NEG");
                }
            } else {
                raise(m, "Wrong number of arguments for NEG");
            }
            break;
        default:
            printf("Unknown opcode\n");
        }

        m->PC = m->nPC;

        //ensures only count instructions are executed
        if (count > 0 && --count == 0)
            return -RUN_OK;
    }

}


int run(struct ami_machine* m, int count)
{
    int ret = _run(m, count);


    if (ret == -RUN_BREAKPOINT) {
        skip_breakpoint();
    } else if (ret == -RUN_HALTED) {
        if (m->opt_graphical) {
            if (m->console_io_status == 0) {
                m->console_io_status = 3;
            }
        } else {
            printf("Program is halted\n");
        }
    }

    return ret;
}

void show_exit_status(struct ami_machine *m)
{

}

