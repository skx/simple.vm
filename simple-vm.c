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
 * The virtual machine will read a series of opcodes an intepret them.
 *
 * Each opcode is an integer between 0-255 inclusive, and will have a variable
 * number of arguments.  The opcodes are divided into groups and allow things
 * such as:
 *
 * * Storing values in registers.
 * * Running mathematical operations on registers.
 * * Jump operations.
 * * Comparisons.
 *
 * For example the addition operation looks like this:
 *
 *    ADD Register-For-Result, Source-Register-One, Source-Register-Two
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
#define BOUNDS_TEST_REGISTER( r ) { if ( r >= REGISTER_COUNT ) \
                                    {  \
                                        printf("Register out of bounds: 0 <= %d >= %d", r, REGISTER_COUNT ); \
                                        exit(1); \
                                    } \
                                  }



/**
 * Helper to convert a two-byte value to an integer in the range
 * 0x0000-0xffff
 */
#define BYTES_TO_ADDR(one,two) (one + ( 256 * two ))

/**
 * Allocate a new virtual machine.
 *
 * The given code will be loaded into the code-area of the machine.
 */
svm_t *svm_new(unsigned char *code, unsigned int size)
{
    svm_t *cpun;
    int i;


    if (!code || !size)
        return NULL;

    cpun = malloc(sizeof(struct svm));
    if (!cpun)
        return NULL;

    memset(cpun, '\0', sizeof(struct svm));

    cpun->esp = 0;
    cpun->size = size;
    cpun->code = code;

    /**
     * Explicitly zero each regiester and set to be a number.
     */
    for (i = 0; i < REGISTER_COUNT; i++)
    {
        cpun->registers[i].type = INTEGER;
        cpun->registers[i].integer = 0;
        cpun->registers[i].string = NULL;
    }

    /**
     * Reset the flags.
     */
    cpun->flags.z = false;

    return cpun;
}


/**
 * Show the content of the various registers.
 */
void svm_dump_registers(svm_t * cpup)
{
    int i;

    printf("Register dump\n");

    for (i = 0; i < REGISTER_COUNT; i++)
    {
        if (cpup->registers[i].type == STRING)
        {
            printf("\tRegister %02d - str: %s\n", i, cpup->registers[i].string);
        } else if (cpup->registers[i].type == INTEGER)
        {
            printf("\tRegister %02d - Decimal:%04d [Hex:%04X]\n", i,
                   cpup->registers[i].integer, cpup->registers[i].integer);
        } else
        {
            printf("\tRegister %02d has unknown type!\n", i);
        }
    }

    if (cpup->flags.z == true)
    {
        printf("\tZ-FLAG:true\n");
    } else
    {
        printf("\tZ-FLAG:false\n");
    }

}


/**
 * Delete a virtual machine.
 */
void svm_free(svm_t * cpup)
{
    if (!cpup)
        return;

    free(cpup);
}


/**
 *  Main virtual machine execution loop
 */
void svm_run(svm_t * cpup)
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


    /**
     * Run continuously - unless we walk off the end of our
     * allocated code, or an exit instruction causes our run
     * flag to be set to zero.
     */
    while (run && (cpup->esp < cpup->size))
    {

      restart:

        if (getenv("DEBUG") != NULL)
            printf("%04x - Parsing OpCode: %d [Hex:%02x]\n", cpup->esp,
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);


        switch (cpup->code[cpup->esp])
        {
        case NOP_OP:
            {
                /**
                 * Nothing to do :)
                 */
                break;
            }
        case INT_PRINT:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type == INTEGER)
                {
                    printf("[stdout] register R%02d = %d [Hex:%04x]\n",
                           cpup->code[reg],
                           cpup->registers[reg].integer, cpup->registers[reg].integer);
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

                if (cpup->registers[reg].type == STRING)
                {
                    printf("[stdout] register R%02d = %s\n", reg,
                           cpup->registers[reg].string);
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

                if (cpup->registers[reg].type == STRING)
                {
                    system(cpup->registers[reg].string);
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

                int offset = BYTES_TO_ADDR(off1, off2);

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

                int offset = BYTES_TO_ADDR(off1, off2);

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

                int offset = BYTES_TO_ADDR(off1, off2);

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



        case INT_STORE:
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

                int value = BYTES_TO_ADDR(val1, val2);

                if (getenv("DEBUG") != NULL)
                    printf("STORE_INT(Reg:%02x) => %04d [Hex:%04x]\n", reg, value, value);

                /* if the register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                cpup->registers[reg].integer = value;
                cpup->registers[reg].type = INTEGER;

                break;
            }

        case INT_TOSTRING:
            {
                /* convert an int-register to a string-register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                int tmp = cpup->registers[reg].integer;

                /* allocate a buffer. */
                cpup->registers[reg].type = STRING;
                cpup->registers[reg].string = malloc(10);

                /* store the string-value */
                memset(cpup->registers[reg].string, '\0', 10);
                sprintf(cpup->registers[reg].string, "%d", tmp);

                break;
            }

        case INC_OP:
            {
                /* increment the contents of a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                if (cpup->registers[reg].type != INTEGER)
                {
                    printf("Tried to decrement aregister which is not an integer\n");
                    exit(1);
                }

                cpup->registers[reg].integer += 1;

                if (cpup->registers[reg].integer == 0)
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


                if (cpup->registers[reg].type != INTEGER)
                {
                    printf("Tried to decrement aregister which is not an integer\n");
                    exit(1);
                }

                cpup->registers[reg].integer -= 1;

                if (cpup->registers[reg].integer == 0)
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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INTEGER) ||
                    (cpup->registers[src2].type != INTEGER))
                {
                    printf("Tried to add two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].integer = cpup->registers[src1].integer +
                    cpup->registers[src2].integer;
                cpup->registers[reg].type = INTEGER;

            /**
             * Overflow?!
             */
                if (cpup->registers[reg].integer == 0)
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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INTEGER) ||
                    (cpup->registers[src2].type != INTEGER))
                {
                    printf("Tried to XOR two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].integer = cpup->registers[src1].integer ^
                    cpup->registers[src2].integer;
                cpup->registers[reg].type = INTEGER;

            /**
             * Zero?
             */
                if (cpup->registers[reg].integer == 0)
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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);


                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INTEGER) ||
                    (cpup->registers[src2].type != INTEGER))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].integer =
                    cpup->registers[src1].integer - cpup->registers[src2].integer;
                cpup->registers[reg].type = INTEGER;

                if (cpup->registers[reg].integer == 0)
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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INTEGER) ||
                    (cpup->registers[src2].type != INTEGER))
                {
                    printf
                        ("Tried to multiply two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].integer = cpup->registers[src1].integer *
                    cpup->registers[src2].integer;
                cpup->registers[reg].type = INTEGER;

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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                if ((cpup->registers[src1].type != INTEGER) ||
                    (cpup->registers[src2].type != INTEGER))
                {
                    printf
                        ("Tried to subtract two registers which do not contain integers\n");
                    exit(1);
                }

                cpup->registers[reg].integer = cpup->registers[src1].integer /
                    cpup->registers[src2].integer;
                cpup->registers[reg].type = INTEGER;

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
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                {
                    free(cpup->registers[reg].string);
                }

            /**
             * Store the new string and set the register type.
             */
                cpup->registers[reg].type = STRING;
                cpup->registers[reg].string = malloc(len + 1);
                memset(cpup->registers[reg].string, '\0', len + 1);

                int i;
                for (i = 0; i < (int) len; i++)
                {
                    cpup->registers[reg].string[i] = cpup->code[cpup->esp];
                    cpup->esp++;
                }

                if (getenv("DEBUG") != NULL)
                    printf("STRING_STORE(Reg:%02x => \"%s\" [%02x bytes]\n", reg,
                           cpup->registers[reg].string, len);

                cpup->esp--;

                break;
            }

        case STRING_TOINT:
            {
                /* convert an string-register to an int-register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);


                if (cpup->registers[reg].type != STRING)
                {
                    printf
                        ("Attempt to read the string value of a register which is an integer\n");
                    exit(1);
                }

                int i = atoi(cpup->registers[reg].string);
                free(cpup->registers[reg].string);

                /* allocate a buffer. */
                cpup->registers[reg].type = INTEGER;
                cpup->registers[reg].integer = i;

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
                if ((cpup->registers[src1].type != STRING) ||
                    (cpup->registers[src2].type != STRING))
                {
                    printf
                        ("Tried to concat two registers which do not contain strings\n");
                    exit(1);
                }

                /**
                 * Allocate RAM for two strings.
                 */
                int len = strlen(cpup->registers[src1].string) +
                    strlen(cpup->registers[src2].string) + 1;

                /**
                 * Zero.
                 */
                char *tmp = malloc(len);
                memset(tmp, '\0', len);

                /**
                 * Assign.
                 */
                sprintf(tmp, "%s%s",
                        cpup->registers[src1].string, cpup->registers[src2].string);


                /* if the destination currently contains a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                cpup->registers[reg].string = tmp;
                cpup->registers[reg].type = STRING;

                break;

            }

        case LOAD_FROM_RAM:
            {
                cpup->esp++;
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int addr = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(addr);

                // Read the value from the RAM
                int val = cpup->code[cpup->registers[addr].integer];

                /* if the destination currently contains a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                cpup->registers[reg].integer = val;
                cpup->registers[reg].type = INTEGER;

                break;
            }
        case STORE_IN_RAM:
            {
                cpup->esp++;
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                cpup->esp++;
                unsigned int addr = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(addr);

                // If the register contains a number then we're golden
                if (cpup->registers[addr].type == INTEGER)
                {
                    int val = cpup->registers[reg].integer;
                    int addr = cpup->registers[addr].integer;

                    cpup->code[addr] = val;
                } else
                {
                    printf("Tried to store a string in RAM!\n");
                    exit(1);
                }

                break;
            }

        case CMP_REG:
            {
                /**
                 * Do two registers have the same value?
                 */
                cpup->esp++;
                unsigned int reg1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg1);

                cpup->esp++;
                unsigned int reg2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg2);

                int equal = 0;

                if (cpup->registers[reg1].type == cpup->registers[reg2].type)
                {
                    if (cpup->registers[reg1].type == STRING)
                    {
                        if (strcmp(cpup->registers[reg1].string,
                                   cpup->registers[reg2].string) == 0 )
                            equal = 1;
                    } else
                    {
                        if (cpup->registers[reg1].integer ==
                            cpup->registers[reg2].integer)
                            equal = 1;
                    }
                } else
                {
                    equal = 0;
                }

                if (equal)
                    cpup->flags.z = true;
                else
                    cpup->flags.z = false;

                break;

            }

        case CMP_IMMEDIATE:
            {
                cpup->esp++;
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(reg);

                /* get the value */
                cpup->esp++;
                unsigned int val1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int val2 = cpup->code[cpup->esp];

                int val = BYTES_TO_ADDR(val1, val2);

                cpup->flags.z = false;

                if (cpup->registers[reg].type == INTEGER)
                {
                    if ((int) cpup->registers[reg].integer == val)
                        cpup->flags.z = true;
                } else
                {
                    printf("Tried to compare an integer to a string!\n");
                    exit(0);
                }

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
