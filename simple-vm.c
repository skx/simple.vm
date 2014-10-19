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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "simple-vm.h"
#include "simple-vm-opcodes.h"


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




/**
 * This function is called if there is an error in handling
 * a bytecode program - such as a mismatched type, or division by zero.
 */
void svm_default_error_handler(svm_t * cpup, char *msg)
{
    /**
     * If the user has registered an error-handler use that instead
     * of this function.
     */
    if (cpup->error_handler)
    {
        (*cpup->error_handler) (msg);

        /**
         * NOTE: If the users' handler doesn't exit then there
         *       WILL BE UNDEFINED BEHAVIOUR
         */
        return;
    }

    /**
     * No error-handler defined, or one was defined which didn't
     * terminate the program.
     *
     * NOTE: Failing to terminate an error-handler will result in undefined
     * results.
     */
    fprintf(stderr, "%s\n", msg);
    exit(1);
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

    if (!code || !size || (size > 0xFFFF))
        return NULL;

    /**
     * Allocate the CPU.
     */
    cpun = malloc(sizeof(struct svm));
    if (!cpun)
        return NULL;
    memset(cpun, '\0', sizeof(struct svm));

    /**
     * Allocate 64k for the program.
     */
    cpun->code = malloc(0xFFFF);
    if (cpun->code == NULL)
    {
        free(cpun);
        return NULL;
    }

    cpun->error_handler = NULL;
    cpun->ip = 0;
    cpun->running = true;
    cpun->size = size;


    /**
     * Zero the RAM we've allocated, and then copy the user's program to
     * the start of it.
     *
     * This means there is a full 64k address-space and the user can
     * have fun writing self-modifying code, & etc.
     *
     */
    memset(cpun->code, '\0', 0xFFFF);
    memcpy(cpun->code, code, size);


    /**
     * Explicitly zero each register and set to be a number.
     */
    for (i = 0; i < REGISTER_COUNT; i++)
    {
        cpun->registers[i].type = INTEGER;
        cpun->registers[i].content.integer = 0;
        cpun->registers[i].content.string = NULL;
    }

    /**
     * Reset the flags.
     */
    cpun->flags.z = false;


    /**
     * Stack is empty.
     */
    cpun->SP = 0;

    /**
     * Setup our default opcode-handlers
     */
    opcode_init(cpun);

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
            printf("\tRegister %02d - str: %s\n", i, cpup->registers[i].content.string);
        } else if (cpup->registers[i].type == INTEGER)
        {
            printf("\tRegister %02d - Decimal:%04d [Hex:%04X]\n", i,
                   cpup->registers[i].content.integer,
                   cpup->registers[i].content.integer);
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
 *
 *  This function will walk through the code passed to the constructor
 * and attempt to execute each bytecode instruction.
 *
 *  If 20+ instructions are fond which can't be executed then the function
 * will abort - otherwise it will keep going until an EXIT instruction is
 * encountered, or the end of the code-block is reached.
 *
 */
void svm_run(svm_t * cpup)
{
    /**
     * How many instructions have we handled?
     */
    int iterations = 0;

    /**
     * If we're called without a valid CPU then we should abort.
     */
    if (!cpup)
        return;


    /**
     * The code will start executing from offset 0.
     */
    cpup->ip = 0;


    /**
     * Run continuously.
     *
     * In practice this means until an EXIT instruction is encountered,
     * which will set the "running"-flag to be false.
     *
     * However the system can cope with IP wrap-around.
     */
    while (cpup->running == true)
    {
        /**
         * Wrap IP on the 64k boundary, if required.
         */
        if (cpup->ip >= 0xFFFF)
            cpup->ip = 0;


        /**
         * Lookup the instruction at the instruction-pointer.
         */
        int opcode = cpup->code[cpup->ip];


        if (getenv("DEBUG") != NULL)
            printf("%04x - Parsing OpCode Hex:%02X\n", cpup->ip, opcode);


        /**
         * Call the opcode implementation, if defined.
         */
        if (cpup->opcodes[opcode] != NULL)
            cpup->opcodes[opcode] (cpup);

        /**
         * NOTE: At this point you might be looking for
         *       a line of the form : cpup->ip += 1;
         *
         *       However this is NOT REQUIRED as each opcode
         *       will have already updated the (global) instruction
         *       pointer.
         *
         *       This is neater because each opcode knows how long it is,
         *       and will probably have bumped the IP to read the register
         *       number, or other arguments.
         *
         */
        iterations++;
    }

    if (getenv("DEBUG") != NULL)
        printf("Executed %u instructions\n", iterations);
}
