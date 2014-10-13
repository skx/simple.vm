/**
 * simple-vm-opcodes.c - Implementation of our opcode-handlers.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "simple-vm.h"
#include "simple-vm-opcodes.h"



/**
 * Trivial helper to test registers are not out of bounds.
 */
#define BOUNDS_TEST_REGISTER( r ) { if ( r >= REGISTER_COUNT )        \
                                    {  \
                                        svm_default_error_handler(svm, "Register out of bounds" ); \
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
 * This is a macro definition for a "math" operation.
 *
 * It's a little longer than I'd usually use for a macro, but it saves
 * all the typing and redundency defining: add, sub, div, mod, xor, or.
 *
 */
#define MATH_OPERATION(function,operator)  _Bool function(struct svm * svm) \
{ \
    /* get the destination register */ \
    unsigned int reg = READ_BYTE(); \
    BOUNDS_TEST_REGISTER(reg); \
\
    /* get the source register */ \
    unsigned int src1 = READ_BYTE(); \
    BOUNDS_TEST_REGISTER(reg); \
\
    /* get the source register */\
    unsigned int src2 = READ_BYTE();\
    BOUNDS_TEST_REGISTER(reg);\
\
    if (getenv("DEBUG") != NULL)\
        printf( #function "(Register:%d = Register:%d " #operator " Register:%d)\n", reg, src1, src2); \
\
    /* if the result-register stores a string .. free it */\
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))\
        free(svm->registers[reg].content.string);\
\
    /* \
     * Ensure both source registers have integer values.\
     */\
    int val1 = get_int_reg(svm, src1);\
    int val2 = get_int_reg(svm, src2);\
\
    /** \
     * Store the result.\
     */\
    svm->registers[reg].content.integer = val1 operator val2; \
    svm->registers[reg].type = INTEGER; \
\
    /**\
     * Zero result? \
     */\
    if (svm->registers[reg].content.integer == 0)\
        svm->flags.z = true;\
        else\
        svm->flags.z = false;\
\
\
    return (false); \
}









/**
 * Helper to return the string-content of a register.
 *
 * NOTE: This function is not exported outside this compilation-unit.
 */
char *get_string_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == STRING)
        return (cpu->registers[reg].content.string);

    svm_default_error_handler(cpu, "The register deesn't contain a string");
    return NULL;
}


/**
 * Helper to return the integer-content of a register.
 *
 * NOTE: This function is not exported outside this compilation-unit.
 */
int get_int_reg(svm_t * cpu, int reg)
{
    if (cpu->registers[reg].type == INTEGER)
        return (cpu->registers[reg].content.integer);

    svm_default_error_handler(cpu, "The register doesn't contain an integer");
    return 0;
}


/**
 * Strings are stored inline in the program-RAM.
 *
 * An example script might contain something like:
 *
 *   store #1, "Steve Kemp"
 *
 * This is encoded as:
 *
 *   OP_STRING_STORE, REG1, LEN1, LEN2, "String data", ...
 *
 * Here we assume the IP is pointing to len1 and we read the length, then
 * the string, and bump the IP as we go.
 *
 * The end result should be we've updated the IP to point past the end
 * of the string, and we've copied it into newly allocated RAM.
 *
 * NOTE: This function is not exported outside this compilation-unit.
 */
char *string_from_stack(svm_t * svm)
{
    /* the string length */
    unsigned int len1 = READ_BYTE();
    unsigned int len2 = READ_BYTE();

    /* build up the length 0-64k */
    int len = BYTES_TO_ADDR(len1, len2);

    /* bump IP one more to point to the start of the string-data. */
    svm->ip += 1;

    /* allocate enough RAM to contain the string. */
    char *tmp = (char *) malloc(len + 1);
    if (tmp == NULL)
        svm_default_error_handler(svm, "RAM allocation failure.");

    /**
     * Zero the allocated memory, and copy the string-contents over.
     *
     * The copy is inefficient - but copes with embedded NULL.
     */
    memset(tmp, '\0', len + 1);
    for (int i = 0; i < (int) len; i++)
    {
        tmp[i] = svm->code[svm->ip];
        svm->ip++;
    }

    return tmp;
}


/**
 ** Start implementation of virtual machine opcodes.
 **
 **/


_Bool op_unknown(svm_t * svm)
{
    int instruction = svm->code[svm->ip];
    printf("%04X - op_unknown(%02X)\n", svm->ip, instruction);
    return false;
}

_Bool op_exit(struct svm * svm)
{
    svm->running = false;
    return false;
}

_Bool op_nop(struct svm * svm)
{
    (void) svm;

    if (getenv("DEBUG") != NULL)
        printf("nop()\n");
    return false;
}

_Bool op_int_store(struct svm * svm)
{
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
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
        free(svm->registers[reg].content.string);

    svm->registers[reg].content.integer = value;
    svm->registers[reg].type = INTEGER;

    return false;
}


_Bool op_int_print(struct svm * svm)
{
    /* get the register number to print */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INT_PRINT(Register %d)\n", reg);

    /* get the register contents. */
    int val = get_int_reg(svm, reg);

    if (getenv("DEBUG") != NULL)
        printf("[STDOUT] Register R%02d => %d [Hex:%04x]\n", reg, val, val);
    else
        printf("%02X", val);


    return (false);
}

_Bool op_int_tostring(struct svm * svm)
{
    /* get the register number to convert */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INT_TOSTRING(Register %d)\n", reg);

    /* get the contents of the register */
    int cur = get_int_reg(svm, reg);

    /* allocate a buffer. */
    svm->registers[reg].type = STRING;
    svm->registers[reg].content.string = malloc(10);

    /* store the string-value */
    memset(svm->registers[reg].content.string, '\0', 10);
    sprintf(svm->registers[reg].content.string, "%d", cur);

    return (false);
}

_Bool op_int_random(struct svm * svm)
{
    /* get the register to save the output to */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INT_RANDOM(Register %d)\n", reg);


    /**
     * If we already have a string in the register delete it.
     */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
    {
        free(svm->registers[reg].content.string);
    }

    /* set the value. */
    svm->registers[reg].type = INTEGER;
    svm->registers[reg].content.integer = rand() % 0xFFFF;

    return (false);
}


_Bool op_string_store(struct svm * svm)
{
    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the string to store */
    char *str = string_from_stack(svm);

    /**
     * If we already have a string in the register delete it.
     */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
    {
        free(svm->registers[reg].content.string);
    }

    /**
     * Now store the new string.
     */
    svm->registers[reg].type = STRING;
    svm->registers[reg].content.string = str;

    if (getenv("DEBUG") != NULL)
        printf("STRING_STORE(Register %d) = '%s'\n", reg, str);

    /* We've tweaked the IP by jumping over the inline-string. */
    return (true);
}

_Bool op_string_print(struct svm * svm)
{
    /* get the reg number to print */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_PRINT(Register %d)\n", reg);

    /* get the contents of the register */
    char *str = get_string_reg(svm, reg);

    /* print */
    if (getenv("DEBUG") != NULL)
        printf("[stdout] register R%02d => %s\n", reg, str);
    else
        printf("%s", str);

    return (false);
}

_Bool op_string_concat(struct svm * svm)
{
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
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
        free(svm->registers[reg].content.string);

    svm->registers[reg].content.string = tmp;
    svm->registers[reg].type = STRING;

    return (false);
}

_Bool op_string_system(struct svm * svm)
{
    /* get the reg */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_SYSTEM(Register %d)\n", reg);

    char *str = get_string_reg(svm, reg);
    system(str);

    return (false);
}

_Bool op_string_toint(struct svm * svm)
{
    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("STRING_TOINT(Register:%d)\n", reg);

    /* get the string and convert to integer */
    char *str = get_string_reg(svm, reg);
    int i = atoi(str);

    /* free the old version */
    free(svm->registers[reg].content.string);

    /* set the int. */
    svm->registers[reg].type = INTEGER;
    svm->registers[reg].content.integer = i;

    return (false);
}


_Bool op_jump_to(struct svm * svm)
{
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

_Bool op_jump_z(struct svm * svm)
{
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

_Bool op_jump_nz(struct svm * svm)
{
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


MATH_OPERATION(op_add, +)       // reg_result = reg1 + reg2 ;
    MATH_OPERATION(op_and, &)   // reg_result = reg1 & reg2 ;
    MATH_OPERATION(op_sub, -)   // reg_result = reg1 - reg2 ;
    MATH_OPERATION(op_mul, *)   // reg_result = reg1 * reg2 ;
    MATH_OPERATION(op_div, /)   // reg_result = reg1 / reg2 ;
    MATH_OPERATION(op_xor, ^)   // reg_result = reg1 ^ reg2 ;
    MATH_OPERATION(op_or, |)    // reg_result = reg1 | reg2 ;
_Bool op_inc(struct svm * svm)
{
    /* get the register number to increment */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("INC_OP(Register %d)\n", reg);

    /* get, incr, set */
    int cur = get_int_reg(svm, reg);
    cur += 1;
    svm->registers[reg].content.integer = cur;

    if (svm->registers[reg].content.integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;

    return (false);
}

_Bool op_dec(struct svm * svm)
{
    /* get the register number to decrement */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    if (getenv("DEBUG") != NULL)
        printf("DEC_OP(Register %d)\n", reg);

    /* get, decr, set */
    int cur = get_int_reg(svm, reg);
    cur -= 1;
    svm->registers[reg].content.integer = cur;

    if (svm->registers[reg].content.integer == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;


    return (false);
}

_Bool op_cmp_reg(struct svm * svm)
{
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
            if (strcmp
                (svm->registers[reg1].content.string,
                 svm->registers[reg2].content.string) == 0)
                svm->flags.z = true;
        } else
        {
            if (svm->registers[reg1].content.integer ==
                svm->registers[reg2].content.integer)
                svm->flags.z = true;
        }
    }

    return (false);
}

_Bool op_cmp_immediate(struct svm * svm)
{
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

_Bool op_cmp_string(struct svm * svm)
{
    /* get the source register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* Now we get the string to compare against from the stack */
    char *str = string_from_stack(svm);

    /* get the string content from the register */
    char *cur = get_string_reg(svm, reg);

    if (getenv("DEBUG") != NULL)
        printf("Comparing register-%d ('%s') - with string '%s'\n", reg, cur, str);

    /* compare */
    if (strcmp(cur, str) == 0)
        svm->flags.z = true;
    else
        svm->flags.z = false;

    /* We've tweaked the IP by jumping over the inline-string. */
    return (true);
}

_Bool op_peek(struct svm * svm)
{
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
        svm_default_error_handler(svm, "Reading from outside RAM");

    /* Read the value from RAM */
    int val = svm->code[adr];

    /* if the destination currently contains a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
        free(svm->registers[reg].content.string);

    svm->registers[reg].content.integer = val;
    svm->registers[reg].type = INTEGER;
    return (false);
}

_Bool op_poke(struct svm * svm)
{
    /* get the destination register */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* get the address to write to from the second register */
    unsigned int addr = READ_BYTE();
    BOUNDS_TEST_REGISTER(addr);

    /* Get the value we're to store. */
    int val = get_int_reg(svm, reg);

    /* Get the address we're to store it in. */
    int adr = get_int_reg(svm, addr);


    if (getenv("DEBUG") != NULL)
        printf("STORE_IN_RAM(Address %04X set to %02X)\n", adr, val);

    if (adr < 0 || adr > 0xffff)
        svm_default_error_handler(svm, "Writing outside RAM");

    /* do the necessary */
    svm->code[adr] = val;
    return (false);
}

_Bool op_memcpy(struct svm * svm)
{
    /* get the register number to store to */
    unsigned int dest_reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(dest_reg);

    /* get the register number to copy from */
    unsigned int src_reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(src_reg);

    /* get the register number with the size */
    unsigned int size_reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(size_reg);

    /**
     * Now handle the copy.
     */
    int src = get_int_reg(svm, src_reg);
    int dest = get_int_reg(svm, dest_reg);
    int size = get_int_reg(svm, size_reg);

    if (getenv("DEBUG") != NULL)
    {
        printf("Copying %4x bytes from %04x to %04X\n", size, src, dest);
    }

    /** Slow, but copes with nulls and allows debugging. */
    for (int i = 0; i < size; i++)
    {
        svm->code[dest + i] = svm->code[src + i];
    }
    return false;
}


_Bool op_stack_push(struct svm * svm)
{
    /* get the register to push */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* Get the value we're to store. */
    int val = get_int_reg(svm, reg);

    if (getenv("DEBUG") != NULL)
        printf("PUSH(Register %d [=%04x])\n", reg, val);

    /* store it */
    svm->SP += 1;
    svm->stack[svm->SP] = val;

    /**
     * Ensure the stack hasn't overflown.
     */
    int sp_size = sizeof(svm->stack) / sizeof(svm->stack[0]);
    if (svm->SP > sp_size)
        svm_default_error_handler(svm, "stack overflow - stack is full");

    return (false);
}

_Bool op_stack_pop(struct svm * svm)
{
    /* get the register to pop */
    unsigned int reg = READ_BYTE();
    BOUNDS_TEST_REGISTER(reg);

    /* ensure we're not outside the stack. */
    if (svm->SP <= 0)
        svm_default_error_handler(svm, "stack overflow - stack is empty");

    /* Get the value from the stack. */
    int val = svm->stack[svm->SP];
    svm->SP -= 1;

    if (getenv("DEBUG") != NULL)
        printf("POP(Register %d) => %04x\n", reg, val);


    /* if the register stores a string .. free it */
    if ((svm->registers[reg].type == STRING) && (svm->registers[reg].content.string))
        free(svm->registers[reg].content.string);

    svm->registers[reg].content.integer = val;
    svm->registers[reg].type = INTEGER;


    return (false);
}

_Bool op_stack_ret(struct svm * svm)
{
    /* ensure we're not outside the stack. */
    if (svm->SP <= 0)
        svm_default_error_handler(svm, "stack overflow - stack is empty");

    /* Get the value from the stack. */
    int val = svm->stack[svm->SP];
    svm->SP -= 1;

    if (getenv("DEBUG") != NULL)
        printf("RET() => %04x\n", val);


    /* update our instruction pointer. */
    svm->ip = val;

    /* We've updated the IP, so we must return true. */
    return (true);
}


_Bool op_stack_call(struct svm * svm)
{
    /**
     * Read the two bytes which will build up the destination
     */
    unsigned int off1 = READ_BYTE();
    unsigned int off2 = READ_BYTE();

    /**
     * Convert to the offset in our code-segment.
     */
    int offset = BYTES_TO_ADDR(off1, off2);


    /**
     * Now we've got to save the address past this instruction
     * on the stack so that the "ret(urn)" instruction will go
     * to the correct place.
     */
    svm->SP += 1;
    svm->stack[svm->SP] = svm->ip + 1;

    /**
     * Now we've saved the return-address we can update the IP
     */
    svm->ip = offset;

    /* We've updated the IP, so we must return true. */
    return (true);
}

/**
 ** End implementation of virtual machine opcodes.
 **
 **/



/**
 * Map the opcodes to the handlers.
 */
void opcode_init(svm_t * svm)
{
    /**
     * Initialize the random seed for the rendom opcode (INT_RANDOM)
     */
    srand(time(NULL));

    /**
     * All instructions will default to unknown.
     */
    for (int i = 0; i < 255; i++)
        svm->opcodes[i] = op_unknown;

    /* early opcodes */
    svm->opcodes[EXIT] = op_exit;
    svm->opcodes[INT_STORE] = op_int_store;
    svm->opcodes[INT_PRINT] = op_int_print;
    svm->opcodes[INT_TOSTRING] = op_int_tostring;
    svm->opcodes[INT_RANDOM] = op_int_random;

    /* jumps */
    svm->opcodes[JUMP_TO] = op_jump_to;
    svm->opcodes[JUMP_NZ] = op_jump_nz;
    svm->opcodes[JUMP_Z] = op_jump_z;

    /* math */
    svm->opcodes[ADD] = op_add;
    svm->opcodes[AND] = op_and;
    svm->opcodes[SUB] = op_sub;
    svm->opcodes[MUL] = op_mul;
    svm->opcodes[DIV] = op_div;
    svm->opcodes[XOR] = op_xor;
    svm->opcodes[OR] = op_or;
    svm->opcodes[INC] = op_inc;
    svm->opcodes[DEC] = op_dec;

    /* strings */
    svm->opcodes[STRING_STORE] = op_string_store;
    svm->opcodes[STRING_PRINT] = op_string_print;
    svm->opcodes[STRING_CONCAT] = op_string_concat;
    svm->opcodes[STRING_SYSTEM] = op_string_system;
    svm->opcodes[STRING_TOINT] = op_string_toint;

    /* comparisons */
    svm->opcodes[CMP_REG] = op_cmp_reg;
    svm->opcodes[CMP_IMMEDIATE] = op_cmp_immediate;
    svm->opcodes[CMP_STRING] = op_cmp_string;

    /* misc */
    svm->opcodes[NOP] = op_nop;

    /* PEEK/POKE */
    svm->opcodes[PEEK] = op_peek;
    svm->opcodes[POKE] = op_poke;
    svm->opcodes[MEMCPY] = op_memcpy;

    /* stack */
    svm->opcodes[STACK_PUSH] = op_stack_push;
    svm->opcodes[STACK_POP] = op_stack_pop;
    svm->opcodes[STACK_RET] = op_stack_ret;
    svm->opcodes[STACK_CALL] = op_stack_call;
}
