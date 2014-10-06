/**
 * simple-vm-opcodes.h - Definitions of our opcode-handlers.
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


#ifndef SIMPLE_VM_OPCODES_H
#define SIMPLE_VM_OPCODES_H 1



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
#define INT_RANDOM    0x04

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
#define CMP_STRING    0x42


/**
 * Misc
 */
#define NOP_OP  0x50


/**
 * RAM things.
 */
#define LOAD_FROM_RAM 0x60
#define STORE_IN_RAM  0x61




_Bool op_exit(void *in);

_Bool op_int_store(void *in);
_Bool op_int_print(void *in);
_Bool op_int_tostring(void *in);
_Bool op_int_random(void *in);

_Bool op_string_store(void *in);
_Bool op_string_print(void *in);
_Bool op_string_concat(void *in);
_Bool op_string_system(void *in);
_Bool op_string_toint(void *in);

_Bool op_jump_to(void *in);
_Bool op_jump_z(void *in);
_Bool op_jump_nz(void *in);

_Bool op_xor(void *in);
_Bool op_add(void *in);
_Bool op_sub(void *in);
_Bool op_mul(void *in);
_Bool op_div(void *in);
_Bool op_inc(void *in);
_Bool op_dec(void *in);

_Bool op_cmp_reg(void *in);
_Bool op_cmp_immediate(void *in);
_Bool op_cmp_string(void *in);

_Bool op_nop(void *in);

_Bool op_load_from_ram(void *in);
_Bool op_store_in_ram(void *in);




/**
 * Initialization function.
 */
void opcode_init(svm_t * cpu);

#endif                          /* SIMPLE_VM_OPCODES_H */
