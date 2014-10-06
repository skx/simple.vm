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
#define BOUNDS_TEST_REGISTER( c, r ) { if ( r >= REGISTER_COUNT )        \
                                    {  \
                                        svm_error_handler( c, "Register out of bounds" ); \
                                    } \
                                  }



/**
 * Helper to convert a two-byte value to an integer in the range
 * 0x0000-0xffff
 */
#define BYTES_TO_ADDR(one,two) (one + ( 256 * two ))





/**
 * This function is called if there is an error in handling
 * some bytecode, or some other part of the system.
 */
void svm_error_handler(svm_t * cpup, char *msg)
{
    if (cpup->error_handler)
    {
        (*cpup->error_handler) (msg);
    } else
    {
        fprintf(stderr, "%s\n", msg);
        exit(1);
    }
}




/**
 * Helper to return the string content of a register.
 */
char *get_string_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == STRING)
        return (cpu->registers[reg].string);
    else
        svm_error_handler(cpu, "The register deesn't contain a string");
    return NULL;
}


/**
 * Helper to return the integer content of a register.
 */
int get_int_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == INTEGER)
        return (cpu->registers[reg].integer);
    else
        svm_error_handler(cpu, "The register doesn't contain an integer");
    return 0;
}



/**
 * Allocate a new virtual machine instance.
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

    cpun->error_handler = NULL;
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
 * Configure a dedicated error-handler.
 *
 * The default error-handler will be called if the bytecode tries to
 * do something crazy, and will merely output a message to the console
 * and terminate.
 *
 * If you wish to handle errors in a GUI system, or similar, you should
 * setup your own error-handler here.
 *
 */
void svm_set_error_handler(svm_t * cpup, void (*fp) (char *msg))
{
    cpup->error_handler = fp;
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
                if (getenv("DEBUG") != NULL)
                    printf("NOP\n");

                /**
                 * Nothing to do :)
                 */
                break;
            }
        case INT_PRINT:
            {
                cpup->esp++;

                /* get the register number to print */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("INT_PRINT(Register %d)\n", reg);


                /* get the register contents. */
                int val = get_int_reg(cpup, reg);

                printf("[stdout] Register R%02d => %d [Hex:%04x]\n", reg, val, val);
                break;
            }

        case STRING_PRINT:
            {
                cpup->esp++;

                /* get the reg number to print */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("STRING_PRINT(Register %d)\n", reg);

                /* get the contents of the register */
                char *str = get_string_reg(cpup, reg);

                /* print */
                printf("[stdout] register R%02d => %s\n", reg, str);
                break;
            }


        case STRING_SYSTEM:
            {
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("STRING_SYSTEM(Register %d)\n", reg);

                char *str = get_string_reg(cpup, reg);
                system(str);
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
                    printf("JUMP_TO(Offset:%d [Hex:%04X]\n", offset, offset);

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
                    printf("JUMP_Z(Offset:%d [Hex:%04X]\n", offset, offset);

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
                    printf("JUMP_NZ(Offset:%d [Hex:%04X]\n", offset, offset);

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
                BOUNDS_TEST_REGISTER(cpup, reg);

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
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("INT_TOSTRING(Register %d)\n", reg);

                /* get the contents of the register */
                int cur = get_int_reg(cpup, reg);

                /* allocate a buffer. */
                cpup->registers[reg].type = STRING;
                cpup->registers[reg].string = malloc(10);

                /* store the string-value */
                memset(cpup->registers[reg].string, '\0', 10);
                sprintf(cpup->registers[reg].string, "%d", cur);

                break;
            }

        case INC_OP:
            {
                /* increment the contents of a register */
                cpup->esp++;

                /* get the reg */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("INC_OP(Register %d)\n", reg);

                /* get, incr, set */
                int cur = get_int_reg(cpup, reg);
                cur += 1;
                cpup->registers[reg].integer = cur;

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
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("DEC_OP(Register %d)\n", reg);

                /* get, decr, set */
                int cur = get_int_reg(cpup, reg);
                cur -= 1;
                cpup->registers[reg].integer = cur;

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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);

                if (getenv("DEBUG") != NULL)
                    printf("ADD_OP(Register:%d = Register:%d + Register:%d)\n", reg, src1,
                           src2);

                /* if the result-register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                int val1 = get_int_reg(cpup, src1);
                int val2 = get_int_reg(cpup, src2);

                /**
                 * Store the result.
                 */
                cpup->registers[reg].integer = val1 + val2;
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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);

                if (getenv("DEBUG") != NULL)
                    printf("XOR_OP(Register:%d = Register:%d ^ Register:%d)\n", reg, src1,
                           src2);


                /* if the result-register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                int val1 = get_int_reg(cpup, src1);
                int val2 = get_int_reg(cpup, src2);

                /**
                 * Store the result.
                 */
                cpup->registers[reg].integer = val1 ^ val2;
                cpup->registers[reg].type = INTEGER;
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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);

                if (getenv("DEBUG") != NULL)
                    printf("SUB_OP(Register:%d = Register:%d - Register:%d)\n", reg, src1,
                           src2);

                /* if the result-register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                int val1 = get_int_reg(cpup, src1);
                int val2 = get_int_reg(cpup, src2);

                /**
                 * Store the result.
                 */
                cpup->registers[reg].integer = val1 - val2;
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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);

                if (getenv("DEBUG") != NULL)
                    printf("MUL_OP(Register:%d = Register:%d * Register:%d)\n", reg, src1,
                           src2);

                /* if the result-register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                int val1 = get_int_reg(cpup, src1);
                int val2 = get_int_reg(cpup, src2);

                /**
                 * Store the result.
                 */
                cpup->registers[reg].integer = val1 * val2;
                cpup->registers[reg].type = INTEGER;

                break;
            }

        case DIV_OP:
            {
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);

                if (getenv("DEBUG") != NULL)
                    printf("DIV_OP(Register:%d = Register:%d / Register:%d)\n", reg, src1,
                           src2);

                /* if the result-register stores a string .. free it */
                if ((cpup->registers[reg].type == STRING)
                    && (cpup->registers[reg].string))
                    free(cpup->registers[reg].string);

                /*
                 * Ensure both source registers have integer values.
                 */
                int val1 = get_int_reg(cpup, src1);
                int val2 = get_int_reg(cpup, src2);

                /**
                 * Store the result.
                 */
                cpup->registers[reg].integer = val1 / val2;
                cpup->registers[reg].type = INTEGER;

                break;
            }

        case STRING_STORE:
            {
                /* store a string in a register */
                cpup->esp++;

                /* get the destination register. */
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

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
                BOUNDS_TEST_REGISTER(cpup, reg);

                if (getenv("DEBUG") != NULL)
                    printf("STRING_TOINT(Register:%d)\n", reg);

                /* get the string and convert to integer */
                char *str = get_string_reg(cpup, reg);
                int i = atoi(str);

                /* free the old version */
                free(cpup->registers[reg].string);

                /* set the int. */
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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int src1 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src1);

                cpup->esp++;
                unsigned int src2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, src2);


                if (getenv("DEBUG") != NULL)
                    printf("STRING_CONCAT(Register:%d = Register:%d + Register:%d)\n",
                           reg, src1, src2);

                /*
                 * Ensure both source registers have string values.
                 */
                char *str1 = get_string_reg(cpup, src1);
                char *str2 = get_string_reg(cpup, src2);

                /**
                 * Allocate RAM for two strings.
                 */
                int len = strlen(str1) + strlen(str2) + 1;

                /**
                 * Zero.
                 */
                char *tmp = malloc(len);
                memset(tmp, '\0', len);

                /**
                 * Assign.
                 */
                sprintf(tmp, "%s%s", str1, str2);


                /* if the destination-register currently contains a string .. free it */
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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int addr = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, addr);

                if (getenv("DEBUG") != NULL)
                    printf
                        ("LOAD_FROM_RAM(Register:%d will contain contents of address %04X)\n",
                         reg, addr);

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
                BOUNDS_TEST_REGISTER(cpup, reg);

                cpup->esp++;
                unsigned int addr = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, addr);

                if (getenv("DEBUG") != NULL)
                    printf("STORE_IN_RAM(Address %04X set to contents of register %d)\n",
                           addr, reg);


                // If the register contains a number then we're golden
                if (cpup->registers[addr].type == INTEGER)
                {
                    int val = cpup->registers[reg].integer;
                    int addr = cpup->registers[addr].integer;

                    cpup->code[addr] = val;
                } else
                {
                    svm_error_handler(cpup, "Tried to store a string in RAM");
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
                BOUNDS_TEST_REGISTER(cpup, reg1);

                cpup->esp++;
                unsigned int reg2 = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg2);

                if (getenv("DEBUG") != NULL)
                    printf("CMP(Register:%d vs Register:%d)\n", reg1, reg2);

                cpup->flags.z = false;

                if (cpup->registers[reg1].type == cpup->registers[reg2].type)
                {
                    if (cpup->registers[reg1].type == STRING)
                    {
                        if (strcmp(cpup->registers[reg1].string,
                                   cpup->registers[reg2].string) == 0)
                            cpup->flags.z = true;
                    } else
                    {
                        if (cpup->registers[reg1].integer ==
                            cpup->registers[reg2].integer)
                            cpup->flags.z = true;
                    }
                }

                break;

            }

        case CMP_IMMEDIATE:
            {
                cpup->esp++;
                unsigned int reg = cpup->code[cpup->esp];
                BOUNDS_TEST_REGISTER(cpup, reg);

                /* get the value */
                cpup->esp++;
                unsigned int val1 = cpup->code[cpup->esp];
                cpup->esp++;
                unsigned int val2 = cpup->code[cpup->esp];

                int val = BYTES_TO_ADDR(val1, val2);

                if (getenv("DEBUG") != NULL)
                    printf("CMP_IMMEDIATE(Register:%d vs %d [Hex:%04X])\n", reg, val,
                           val);

                cpup->flags.z = false;

                int cur = (int) get_int_reg(cpup, reg);

                if (cur == val)
                    cpup->flags.z = true;

                break;

            }

        default:
            printf("UNKNOWN INSTRUCTION: %d [Hex: %2X]\n",
                   cpup->code[cpup->esp], cpup->code[cpup->esp]);

            unknown += 1;

            if (unknown >= 20)
            {
                svm_error_handler(cpup, "Aborting due to too many unknown instructions");
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
