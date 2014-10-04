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
#define STORE_INT     0x01
#define PRINT_INT     0x02


/**
 * Jump things.
 */
#define JUMP_TO       0x10
#define JUMP_Z        0x11
#define JUMP_NZ       0x12


/**
 * Math things.
 */
#define ADD_OP 0x21
#define SUB_OP 0x22
#define MUL_OP 0x23
#define DIV_OP 0x24
#define INC_OP 0x25
#define DEC_OP 0x26

/**
 * String things.
 */

#define STRING_STORE  0x30
#define STRING_PRINT  0x31
#define STRING_CONCAT 0x32
#define STRING_SYSTEM 0x33


/**
 * The type of content a register has.
 */
enum TypeTag { INT, STR };

/**
 * A single register, which may be used to store a string or an integer.
 */
typedef struct registers {
    unsigned int num;
    char *str;
    enum TypeTag type;
} reg_t;



/**
 * Flags.
 *
 * The add/sub instructions set the Z flag if the result is zero.
 *
 * This flag can then be used for the JMP_Z and JUMP_NZ instructions.
 */
typedef struct flags {
    _Bool z;
} flag_t;


/**
 * The CPU type, which contains:
 *
 * 1.  An array of registers.
 * 2.  An instruction pointer.
 * 3.  A set of code to execute - which has a size.
 *
 */
typedef struct cpu {
    reg_t registers[REGISTER_COUNT];
    flag_t flags;
    unsigned int esp;
    unsigned int size;
    unsigned char *code;

    unsigned int labels[255];
} cpu_t;



/**
 * Allocate a new CPU.
 */
cpu_t *cpu_new(unsigned char *code, unsigned int size);


/**
 * Dump the registers of the CPU.
 */
void cpu_dump_registers(cpu_t * cpup);


/**
 * Delete a CPU.
 */
void cpu_del(cpu_t * cpup);

/**
 *  Main virtual machine execution loop
 */
void cpu_run(cpu_t * cpup);


#endif                          /* SIMPLE_VM_H */
