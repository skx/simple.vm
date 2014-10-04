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
#define OPCODE_EXIT   0x00      /* EXIT() - Helpfully zero. */

#define STORE_STRING  0x01      /* STORE_STRING( RegN, LEN, "STRING" ) */
#define STORE_INT     0x02      /* STORE_INT( RegN, val ) */

#define PRINT_STRING  0x03      /* PRINT_STRING( RegN ) */
#define PRINT_INT     0x04      /* PRINT_INT( RegN ) */

#define JUMP_TO       0x06      /* JUMP_TO address */
#define JUMP_Z        0x07      /* JUMP_Z address */
#define JUMP_NZ       0x08      /* JUMP_NZ address */

#define ADD_OP 0x09
#define SUB_OP 0x0A
#define MUL_OP 0x0B
#define DIV_OP 0x0C


#define SYSTEM_STRING 0x0D

#define INC_OP 0x0E
#define DEC_OP 0x0F


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
