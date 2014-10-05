/**
 * simple-vm.h - Public header-file for simple virtual machine.
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


#ifndef SIMPLE_VM_H
#define SIMPLE_VM_H 1



/**
 * Count of registers.
 */
#define REGISTER_COUNT 10


#ifndef _Bool
#define _Bool short
#define true   1
#define false  0
#endif


/***
 * Our "opcodes".
 */
#define OPCODE_EXIT   0x00


/**
 * Integer things.
 */
#define INT_STORE     0x01
#define INT_PRINT     0x02
#define INT_TOSTRING  0x03


/**
 * Jump things.
 */
#define JUMP_TO       0x10
#define JUMP_Z        0x11
#define JUMP_NZ       0x12


/**
 * Math things.
 */
#define XOR_OP 0x20
#define ADD_OP 0x21
#define SUB_OP 0x22
#define MUL_OP 0x23
#define DIV_OP 0x24
#define INC_OP 0x25
#define DEC_OP 0x26

/**
 * String things.
 */
#define STRING_STORE   0x30
#define STRING_PRINT   0x31
#define STRING_CONCAT  0x32
#define STRING_SYSTEM  0x33
#define STRING_TOINT   0x34


/**
 * Comparisons
 */
#define CMP_REG       0x40
#define CMP_IMMEDIATE 0x41


/**
 * Misc
 */
#define NOP_OP  0x50


/**
 * RAM things.
 */
#define LOAD_FROM_RAM 0x60
#define STORE_IN_RAM  0x61



/**
 * A single register, which may be used to store a string or an integer.
 */
typedef struct registers {
    union {
        unsigned int integer;
        char *string;
    };
    char *str;
    enum { INTEGER, STRING } type;
} reg_t;



/**
 * Flags.
 *
 * The add/sub/incr/dec/cmp instructions set the Z flag if the result is zero.
 *
 * This flag can then be used for the JMP_Z and JUMP_NZ instructions.
 */
typedef struct flags {
    _Bool z;
} flag_t;


/**
 * The Simple Virtual Machine type, which contains:
 *
 * 1.  An array of registers.
 * 2.  A set of virtual-flags.
 * 3.  An instruction pointer.
 * 4.  A set of code to execute - which has a size.
 *
 */
typedef struct svm {
    reg_t registers[REGISTER_COUNT];
    flag_t flags;
    unsigned int esp;
    unsigned int size;
    unsigned char *code;
} svm_t;



/**
 * Allocate a new virtual machine.
 */
svm_t *svm_new(unsigned char *code, unsigned int size);


/**
 * Dump the virtual machine registers.
 */
void svm_dump_registers(svm_t * cpup);


/**
 * Delete a virtual machine.
 */
void svm_free(svm_t * cpup);


/**
 *  Main virtual machine execution loop
 */
void svm_run(svm_t * cpup);


#endif                          /* SIMPLE_VM_H */
