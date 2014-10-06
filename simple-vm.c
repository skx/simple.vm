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
#define BOUNDS_TEST_REGISTER( r ) { if ( r >= REGISTER_COUNT )        \
                                    {  \
                                        svm_error_handler(svm, "Register out of bounds" ); \
                                    } \
                                  }



/**
 * Read and return the next byte from the current instruction-pointer.
 */
#define READ_BYTE() (svm->code[++svm->ip])



/**
 * Helper to convert a two-byte value to an integer in the range 0x0000-0xffff
 */
#define BYTES_TO_ADDR(one,two) (one + ( 256 * two ))






/**
 * This function is called if there is an error in handling
 * a bytecode program - such as a mismatched type, or division by zero.
 *
 * NOTE: This is not exported outside this compilation-unit.
 *
 */
void svm_error_handler(svm_t * cpup, char *msg)
{
    /**
     * If the user has registered an error-handler use that instead
     * of this function.
     */
    if (cpup->error_handler)
    {
        (*cpup->error_handler) (msg);
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
 * Helper to return the string-content of a register.
 *
 * NOTE: This is not exported outside this compilation-unit.
 */
char *get_string_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == STRING)
        return (cpu->registers[reg].string);

    svm_error_handler(cpu, "The register deesn't contain a string");
    return NULL;
}


/**
 * Helper to return the integer-content of a register.
 *
 * NOTE: This is not exported outside this compilation-unit.
 */
int get_int_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == INTEGER)
        return (cpu->registers[reg].integer);

    svm_error_handler(cpu, "The register doesn't contain an integer");
    return 0;
}



/**
 ** Start implementation of virtual machine opcodes.
 **
 ** These are not exported outside this compilation-unit, so they
 ** must be defined before they can be referred to in the `svm_new`
 ** function.
 **
 **/
_Bool op_exit(void *in)
{
    svm_t *svm = (svm_t *) in;
    svm->running = false;
    return false;
}

_Bool op_nop(void *in)
{
    svm_t *svm = (svm_t *) in;
    (void) svm;

    if (getenv("DEBUG") != NULL)
        printf("nop()\n");
    return false;
}


_Bool op_int_store(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the register number to store in */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the value */
    unsigned int val1 = READ_BYTE();
    unsigned int val2 = READ_BYTE();
    int value = BYTES_TO_ADDR(val1, val2);

    if (getenv("DEBUG") != NULL)
        printf("STORE_INT(Reg:%02x) => %04d [Hex:%04x]\n", reg, value, value);

    /* if the register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    svm->registers[reg].integer = value;
    svm->registers[reg].type = INTEGER;

    return false;
}


_Bool op_int_print(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the register number to print */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INT_PRINT(Register %d)\n", reg);

    /* get the register contents. */
    int val = get_int_reg(svm, reg);
    printf("[stdout] Register R%02d => %d [Hex:%04x]\n", reg, val, val);

    return (false);
}

_Bool op_int_tostring(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the register number to convert */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INT_TOSTRING(Register %d)\n", reg);

    /* get the contents of the register */
    int cur = get_int_reg(svm, reg);

    /* allocate a buffer. */
    svm->registers[reg].type = STRING;
    svm->registers[reg].string = malloc(10);

    /* store the string-value */
    memset(svm->registers[reg].string, '\0', 10);
    sprintf(svm->registers[reg].string, "%d", cur);

    return (false);
}


_Bool op_string_store(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* the string length - max 255 - FIXME */
    unsigned int len = READ_BYTE();

    /* bump IP one more. */
    svm->ip += 1;

    /**
     * If we already have a string in the register delete it.
     */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
    {
        free(svm->registers[reg].string);
    }

    /**
     * Store the new string and set the register type.
     */
    svm->registers[reg].type = STRING;
    svm->registers[reg].string = malloc(len + 1);
    memset(svm->registers[reg].string, '\0', len + 1);

    /**
     * Inefficient - but copes with embedded NULL.
     */
    int i;
    for (i = 0; i < (int) len; i++)
    {
        svm->registers[reg].string[i] = svm->code[svm->ip];
        svm->ip++;
    }

    if (getenv("DEBUG") != NULL)
        printf("STRING_STORE(Reg:%02x => \"%s\" [%02x bytes]\n", reg,
               svm->registers[reg].string, len);

    svm->ip--;
    return (false);
}

_Bool op_string_print(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the reg number to print */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_PRINT(Register %d)\n", reg);

    /* get the contents of the register */
    char *str = get_string_reg(svm, reg);

    /* print */
    printf("[stdout] register R%02d => %s\n", reg, str);

    return (false);
}

_Bool op_string_concat(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_CONCAT(Register:%d = Register:%d + Register:%d)\n",
               reg, src1, src2);

    /*
     * Ensure both source registers have string values.
     */
    char *str1 = get_string_reg(svm, src1);
    char *str2 = get_string_reg(svm, src2);

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
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    svm->registers[reg].string = tmp;
    svm->registers[reg].type = STRING;

    return (false);
}

_Bool op_string_system(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the reg */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_SYSTEM(Register %d)\n", reg);

    char *str = get_string_reg(svm, reg);
    system(str);

    return (false);
}

_Bool op_string_toint(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_TOINT(Register:%d)\n", reg);

    /* get the string and convert to integer */
    char *str = get_string_reg(svm, reg);
    int i = atoi(str);

    /* free the old version */
    free(svm->registers[reg].string);

    /* set the int. */
    svm->registers[reg].type = INTEGER;
    svm->registers[reg].integer = i;

    return (false);
}


_Bool op_jump_to(void *in)
{
    svm_t *svm = (svm_t *) in;

    /**
     * Read the two bytes which will build up the destination
     */
    unsigned int off1 = READ_BYTE();
    unsigned int off2 = READ_BYTE();

    /**
     * Convert to the offset in our code-segment.
     */
    int offset = BYTES_TO_ADDR(off1, off2);

    if (getenv("DEBUG") != NULL)
        printf("JUMP_TO(Offset:%d [Hex:%04X]\n", offset, offset);

    svm->ip = offset;
    return (true);
}

_Bool op_jump_z(void *in)
{
    svm_t *svm = (svm_t *) in;

    /**
     * Read the two bytes which will build up the destination
     */
    unsigned int off1 = READ_BYTE();
    unsigned int off2 = READ_BYTE();

    /**
     * Convert to the offset in our code-segment.
     */
    int offset = BYTES_TO_ADDR(off1, off2);

    if (getenv("DEBUG") != NULL)
        printf("JUMP_Z(Offset:%d [Hex:%04X]\n", offset, offset);

    if (svm->flags.z)
    {
        svm->ip = offset;
        return true;
    }

    return (false);
}

_Bool op_jump_nz(void *in)
{
    svm_t *svm = (svm_t *) in;

    /**
     * Read the two bytes which will build up the destination
     */
    unsigned int off1 = READ_BYTE();
    unsigned int off2 = READ_BYTE();

    /**
     * Convert to the offset in our code-segment.
     */
    int offset = BYTES_TO_ADDR(off1, off2);

    if (getenv("DEBUG") != NULL)
        printf("JUMP_NZ(Offset:%d [Hex:%04X]\n", offset, offset);

    if (!svm->flags.z)
    {
        svm->ip = offset;
        return true;
    }

    return false;
}

_Bool op_xor(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);


    if (getenv("DEBUG") != NULL)
        printf("XOR_OP(Register:%d = Register:%d ^ Register:%d)\n", reg, src1, src2);


    /* if the result-register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    /*
     * Ensure both source registers have integer values.
     */
    int val1 = get_int_reg(svm, src1);
    int val2 = get_int_reg(svm, src2);

    /**
     * Store the result.
     */
    svm->registers[reg].integer = val1 ^ val2;
    svm->registers[reg].type = INTEGER;
    svm->registers[reg].type = INTEGER;

    /**
     * Zero?
     */
    if (svm->registers[reg].integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;

    return (false);
}

_Bool op_add(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("ADD_OP(Register:%d = Register:%d + Register:%d)\n", reg, src1, src2);

    /* if the result-register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    /*
     * Ensure both source registers have integer values.
     */
    int val1 = get_int_reg(svm, src1);
    int val2 = get_int_reg(svm, src2);

    /**
     * Store the result.
     */
    svm->registers[reg].integer = val1 + val2;
    svm->registers[reg].type = INTEGER;

    /**
     * Overflow?!
     */
    if (svm->registers[reg].integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;


    return (false);
}

_Bool op_sub(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("SUB_OP(Register:%d = Register:%d - Register:%d)\n", reg, src1, src2);

    /* if the result-register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    /*
     * Ensure both source registers have integer values.
     */
    int val1 = get_int_reg(svm, src1);
    int val2 = get_int_reg(svm, src2);

    /**
     * Store the result.
     */
    svm->registers[reg].integer = val1 - val2;
    svm->registers[reg].type = INTEGER;

    if (svm->registers[reg].integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;

    return (false);
}

_Bool op_mul(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);


    if (getenv("DEBUG") != NULL)
        printf("MUL_OP(Register:%d = Register:%d * Register:%d)\n", reg, src1, src2);

    /* if the result-register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    /*
     * Ensure both source registers have integer values.
     */
    int val1 = get_int_reg(svm, src1);
    int val2 = get_int_reg(svm, src2);

    /**
     * Store the result.
     */
    svm->registers[reg].integer = val1 * val2;
    svm->registers[reg].type = INTEGER;

    return (false);
}

_Bool op_div(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the source register */
    unsigned int src2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (src2 == 0)
        svm_error_handler(svm, "Attempted division by zero.");

    if (getenv("DEBUG") != NULL)
        printf("DIV_OP(Register:%d = Register:%d / Register:%d)\n", reg, src1, src2);

    /* if the result-register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    /*
     * Ensure both source registers have integer values.
     */
    int val1 = get_int_reg(svm, src1);
    int val2 = get_int_reg(svm, src2);

    /**
     * Store the result.
     */
    svm->registers[reg].integer = val1 / val2;
    svm->registers[reg].type = INTEGER;


    return (false);
}

_Bool op_inc(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the register number to increment */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INC_OP(Register %d)\n", reg);

    /* get, incr, set */
    int cur = get_int_reg(svm, reg);
    cur += 1;
    svm->registers[reg].integer = cur;

    if (svm->registers[reg].integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;

    return (false);
}

_Bool op_dec(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the register number to decrement */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("DEC_OP(Register %d)\n", reg);

    /* get, decr, set */
    int cur = get_int_reg(svm, reg);
    cur -= 1;
    svm->registers[reg].integer = cur;

    if (svm->registers[reg].integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;


    return (false);
}

_Bool op_cmp_reg(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the source register */
    unsigned int reg1 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg1);

    /* get the source register */
    unsigned int reg2 = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg2);

    if (getenv("DEBUG") != NULL)
        printf("CMP(Register:%d vs Register:%d)\n", reg1, reg2);

    svm->flags.z = false;

    if (svm->registers[reg1].type == svm->registers[reg2].type)
    {
        if (svm->registers[reg1].type == STRING)
        {
            if (strcmp(svm->registers[reg1].string, svm->registers[reg2].string) == 0)
                svm->flags.z = true;
        } else
        {
            if (svm->registers[reg1].integer == svm->registers[reg2].integer)
                svm->flags.z = true;
        }
    }

    return (false);
}

_Bool op_cmp_immediate(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the source register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the integer to compare with */
    unsigned int val1 = READ_BYTE();
    unsigned int val2 = READ_BYTE();
    int val = BYTES_TO_ADDR(val1, val2);

    if (getenv("DEBUG") != NULL)
        printf("CMP_IMMEDIATE(Register:%d vs %d [Hex:%04X])\n", reg, val, val);

    svm->flags.z = false;

    int cur = (int) get_int_reg(svm, reg);

    if (cur == val)
        svm->flags.z = true;

    return (false);
}

_Bool op_load_from_ram(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the address to read from the second register */
    unsigned int addr = READ_BYTE();
    BOUNDS_TEST_REGISTER(addr);

    if (getenv("DEBUG") != NULL)
        printf
            ("LOAD_FROM_RAM(Register:%d will contain contents of address %04X)\n",
             reg, addr);

    /* get the address from the register */
    int adr = get_int_reg(svm, addr);
    if (adr < 0 || adr > 0xffff)
        svm_error_handler(svm, "Reading from outside RAM");

    /* Read the value from RAM */
    int val = svm->code[adr];

    /* if the destination currently contains a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].string))
        free(svm->registers[reg].string);

    svm->registers[reg].integer = val;
    svm->registers[reg].type = INTEGER;
    return (false);
}

_Bool op_store_in_ram(void *in)
{
    svm_t *svm = (svm_t *) in;

    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the address to write to from the second register */
    unsigned int addr = READ_BYTE();
    BOUNDS_TEST_REGISTER(addr);

    if (getenv("DEBUG") != NULL)
        printf("STORE_IN_RAM(Address %04X set to contents of register %d)\n", addr, reg);

    /* Get the value we're to store. */
    int val = get_int_reg(svm, reg);

    /* Get the address we're to store it in. */
    int adr = get_int_reg(svm, addr);

    if (adr < 0 || adr > 0xffff)
        svm_error_handler(svm, "Writing outside RAM");

    /* do the necessary */
    svm->code[adr] = val;
    return (false);
}


/**
 ** End implementation of virtual machine opcodes.
 **
 ** These are not exported outside this compilation-unit, so they
 ** must be defined before they can be referred to in the `svm_new`
 ** function.
 **
 **/



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
    cpun->ip = 0;
    cpun->running = true;
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

    /**
     * Setup our default opcode-handlers
     */
    cpun->opcodes[OPCODE_EXIT] = op_exit;
    cpun->opcodes[NOP_OP] = op_nop;

    cpun->opcodes[INT_STORE] = op_int_store;
    cpun->opcodes[INT_PRINT] = op_int_print;
    cpun->opcodes[INT_TOSTRING] = op_int_tostring;

    cpun->opcodes[STRING_STORE] = op_string_store;
    cpun->opcodes[STRING_PRINT] = op_string_print;
    cpun->opcodes[STRING_CONCAT] = op_string_concat;
    cpun->opcodes[STRING_SYSTEM] = op_string_system;
    cpun->opcodes[STRING_TOINT] = op_string_toint;

    cpun->opcodes[JUMP_TO] = op_jump_to;
    cpun->opcodes[JUMP_NZ] = op_jump_nz;
    cpun->opcodes[JUMP_Z] = op_jump_z;

    cpun->opcodes[LOAD_FROM_RAM] = op_load_from_ram;
    cpun->opcodes[STORE_IN_RAM] = op_store_in_ram;

    cpun->opcodes[ADD_OP] = op_add;
    cpun->opcodes[SUB_OP] = op_sub;
    cpun->opcodes[MUL_OP] = op_mul;
    cpun->opcodes[DIV_OP] = op_div;
    cpun->opcodes[XOR_OP] = op_xor;
    cpun->opcodes[INC_OP] = op_inc;
    cpun->opcodes[DEC_OP] = op_dec;

    cpun->opcodes[CMP_REG] = op_cmp_reg;
    cpun->opcodes[CMP_IMMEDIATE] = op_cmp_immediate;
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
     * Run continuously - unless we walk off the end of our
     * allocated code, or an EXIT instruction causes our run
     * flag to be set to zero.
     */
    while ((cpup->running == true) && (cpup->ip < cpup->size))
    {
        if (!cpup->running)
            break;


        /**
         * Lookup the instruction at the instruction-pointer.
         */
        int opcode = cpup->code[cpup->ip];


        if (getenv("DEBUG") != NULL)
            printf("%04x - Parsing OpCode Hex:%02X\n", cpup->ip, opcode);


        /**
         * Did the instruction change the IP?
         */
        _Bool jumped = false;

        /**
         * If we have an implementation for that opcode, then
         * call it.
         */
        if (cpup->opcodes[opcode] != NULL)
        {

            /**
             * Call the handler
             */
            jumped = cpup->opcodes[opcode] (cpup);
            iterations++;
        } else
        {
            printf("Unknown opcode: %2X\n", opcode);
        }

        /**
         * Advance to the next instruction - UNLESS the IP
         * has been changed during the handler.
         */
        if (jumped == false)
            cpup->ip++;
    }

    if (getenv("DEBUG") != NULL)
        printf("Executed %u instructions\n", iterations);
}
