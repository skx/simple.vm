/**
 * simple-vm.c - Implementation-file for simple virtual machine.
 *
 * Copyright (c) 2014 by Steve Kemp.  All rights reserved.
 *
 **
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991, or (at your
 * option) any later version.
 *
 * On Debian GNU/Linux systems, the complete text of version 2 of the GNU
 * General Public License can be found in `/usr/share/common-licenses/GPL-2'
 *
 **
 *
 */


#include <string.h>
#include <inttypes.h>
#include <unistd.h>


#include "simple-vm.h"


/**
 *
 * This is a simple virtual-machine which uses registers, rather than
 * the stack.
 *
 * The virtual machine will execute the contents of the named program-file,
 * by reading the file and parsing the binary-opcodes.
 *
 * Each opcode is an integer between 0-255 inclusive, and will have a variable
 * number of arguments.  The opcodes are divided into classes which are
 * broadly:
 *
 * * Storing values in registers.
 * * Running mathematical operations on registers.
 * * etc.
 *
 * For example the addition operation looks like this:
 *
 *    ADD Register-For-Result, Source-Register-One, Source-REgister-Two
 *
 * And the storing of a number in a register looks like this:
 *
 *   STORE Register-For-Result NUMBER
 *
 * The machine has 10 registers total, numbered 00-09.
 *
 *
 * Steve
 *
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



/**
 * Trivial helper to test registers are not out of bounds.
 */
#define BOUNDS_TEST_REGISTER( r ) { if ( r >= REGISTER_COUNT )  { printf("Register out of bounds: 0 <= %d >= %d", reg, REGISTER_COUNT ); exit(1); } }



/**
 * Allocate a new CPU.
 */
cpu_t *cpu_new(unsigned char *code, unsigned int size)
{
    cpu_t *cpun;
    int i;


    if (!code || !size)
        return NULL;

    cpun = malloc(sizeof(struct cpu));
    if (!cpun)
        return NULL;

    memset(cpun, '\0', sizeof(struct cpu));

    cpun->esp = 0;
    cpun->size = size;
    cpun->code = code;

    /**
     * Explicitly zero each regiester and set to be a number.
     */
    for (i = 0; i < REGISTER_COUNT; i++)
    {
        cpun->registers[i].type = INT;
        cpun->registers[i].num = 0;
        cpun->registers[i].str = NULL;
    }

    return cpun;
}


/**
 * Show the content of the various registers.
 */
void cpu_dump_registers(cpu_t * cpup)
{
    int i;

    printf("Register dump\n");

    for (i = 0; i < REGISTER_COUNT; i++)
    {
        if (cpup->registers[i].type == STR)
        {
            printf("\tRegister %02d - str: %s\n", i, cpup->registers[i].str);
        } else if (cpup->registers[i].type == INT)
        {
            printf("\tRegister %02d - Decimal:%04d [Hex:%04X]\n", i,
                   cpup->registers[i].num, cpup->registers[i].num);
        } else
        {
            printf("\tRegister %02d has unknown type!\n", i);
        }
    }
}


/**
 * Delete a CPU.
 */
void cpu_del(cpu_t * cpup)
{
    if (!cpup)
        return;

    free(cpup);
}


/**
 *  Main virtual machine execution loop
 */
void cpu_run(cpu_t * cpup)
{
    /**
     * How many instructions this run handled.
     */
    int iterations = 0;

    /**
     * How many unrecognized instructions.
     */
    int unknown = 0;


    /**
     * Are we running?
     */
    int run = 1;


    /**
     * If this is called without a valid CPU then we should abort.
     */
    if (!cpup)
        return;


    /**
     * The code will start executing from offset 0.
     */
    cpup->esp = 0;


    while (run && (cpup->esp < cpup->size))
    {

      restart:

        if (getenv("DEBUG") != NULL)
            printf("%04x - Parsing OpCode: %d [Hex:%02x]\n", cpup->esp,
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);


        switch (cpup->code[cpup->esp])
        {

        case PRINT_INT:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type == INT)
                {
                    printf("[stdout] register R%02d = %d [Hex:%04x]\n",
                           cpup->code[reg],
                           cpup->registers[reg].num, cpup->registers[reg].num);
                } else
                {
                    printf
                        ("ERROR Tried to print integer contents of register %02x but it is a string\n",
                         reg);
                }
                break;
            }

        case STRING_PRINT:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type == STR)
                {
                    printf("[stdout] register R%02d = %s\n", reg,
                           cpup->registers[reg].str);
                } else
                {
                    printf
                        ("ERROR Tried to print string contents of register %02x but it is an integer\n",
                         reg);
                }
                break;
            }


        case STRING_SYSTEM:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type == STR)
                {
                    system(cpup->registers[reg].str);
                } else
                {
                    printf
                        ("ERROR Tried to execute the contents register %02x but it is an integer\n",
                         reg);
                }
                break;
            }

        case JUMP_TO:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x]\n", offset, offset);

                cpup->esp = offset;
                goto restart;
                break;
            }



        case JUMP_Z:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x] if Z\n", offset,
                           offset);

                if (cpup->flags.z)
                {
                    cpup->esp = offset;
                    goto restart;
                }
                break;
            }


        case JUMP_NZ:
            {
                cpup->esp++;

                short off1 = cpup->code[cpup->esp];
                cpup->esp++;
                short off2 = cpup->code[cpup->esp];

                int offset = off1 + (256 * off2);

                if (getenv("DEBUG") != NULL)
                    printf("Should jump to offset %04d [Hex:%04x] if NOT Z\n", offset,
                           offset);

                if (!cpup->flags.z)
                {
                    cpup->esp = offset;
                    goto restart;
                }
                break;
            }



        case STORE_INT:
            {
                /* store an int in a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                /* get the value */
                cpup->esp++;
                unsigned int val1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int val2 = cpup->code[cpup->esp];

                int value = val1 + (256 * val2);

                if (getenv("DEBUG") != NULL)
                    printf("STORE_INT(Reg:%02x) => %04d [Hex:%04x]\n", reg, value, value);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                cpup->registers[reg].num = value;
                cpup->registers[reg].type = INT;

                break;
            }

        case INC_OP:
            {
                /* increment the contents of a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type != INT)
                {
                    printf("Tried to decrement aregister which is not an integer\n");
                    exit(1);
                }

                cpup->registers[reg].num += 1;

                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;


                break;
            }
        case DEC_OP:
            {
                /* increment the contents of a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);


                if (cpup->registers[reg].type != INT)
                {
                    printf("Tried to decrement aregister which is not an integer\n");
                    exit(1);
                }

                cpup->registers[reg].num -= 1;

                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;


                break;
            }
        case ADD_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf("Tried to add two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num +
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

            /**
             * Overflow?!
             */
                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;


                break;
            }

        case XOR_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf("Tried to XOR two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num ^
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

            /**
             * Zero?
             */
                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;


                break;
            }

        case SUB_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);


                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num =
                    cpup->registers[src1].num - cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                if (cpup->registers[reg].num == 0)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;

                break;
            }

        case MUL_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to multiply two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num *
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                break;
            }

        case DIV_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);


                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INT) ||
                    (cpup->registers[src2].type != INT))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].num = cpup->registers[src1].num /
                    cpup->registers[src2].num;
                cpup->registers[reg].type = INT;

                break;
            }

        case STRING_STORE:
            {
                /* store a string in a register */
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;

                /* get the length */
                unsigned int len = cpup->code[cpup->esp];
                cpup->esp++;

            /**
             * If we already have a string in the register delete it.
             */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                {
                    free(cpup->registers[reg].str);
                }

            /**
             * Store the new string and set the register type.
             */
                cpup->registers[reg].type = STR;
                cpup->registers[reg].str = malloc(len + 1);
                memset(cpup->registers[reg].str, '\0', len + 1);

                int i;
                for (i = 0; i < (int) len; i++)
                {
                    cpup->registers[reg].str[i] = cpup->code[cpup->esp];
                    cpup->esp++;
                }

                if (getenv("DEBUG") != NULL)
                    printf("STRING_STORE(Reg:%02x => \"%s\" [%02x bytes]\n", reg,
                           cpup->registers[reg].str, len);

                cpup->esp--;

                break;
            }

        case OPCODE_EXIT:
            {
                if (getenv("DEBUG") != NULL)
                    printf("exit\n");
                run = 0;
                break;
            }

        case STRING_CONCAT:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(src2);


                /*
                 * Ensure both source registers have string values.
                 */
                if ((cpup->registers[src1].type != STR) ||
                    (cpup->registers[src2].type != STR))
                {
                    printf
                        ("Tried to concat two registers which do not contain strings\n");
                    exit(1);
                }

                /**
                 * Allocate RAM for two strings.
                 */
                int len = strlen(cpup->registers[src1].str) +
                    strlen(cpup->registers[src2].str) + 1;

                /**
                 * Zero.
                 */
                char *tmp = malloc(len);
                memset(tmp, '\0', len);

                /**
                 * Assign.
                 */
                sprintf(tmp, "%s%s",
                        cpup->registers[src1].str, cpup->registers[src2].str);


                /* if the destination currently contains a string .. free it */
                if ((cpup->registers[reg].type == STR) && (cpup->registers[reg].str))
                    free(cpup->registers[reg].str);

                cpup->registers[reg].str = tmp;
                cpup->registers[reg].type = STR;

                break;

            }

        default:
            printf("UNKNOWN INSTRUCTION: %d [Hex: %2X]\n",
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);

            unknown += 1;

            if (unknown >= 20)
            {
                printf("Aborting due to too many unknown instructions\n");
                run = 0;
            }
            break;
        }
        cpup->esp++;

        iterations++;
    }

    if (getenv("DEBUG") != NULL)
        printf("Executed %u instructions\n", iterations);
}
