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



/**
 * A single register.
 *
 * Our registers contain a simple union which allows them to store either
 * a string or an integer.
 *
 */
typedef struct registers {
    union {
        unsigned int integer;
        char *string;
    } content;
    char *str;
    enum { INTEGER, STRING } type;
} reg_t;



/**
 * Flags.
 *
 * The various mathematical operations (such as add/sub/xor) will set the
 * Z flag to be true if their result is zero.
 *
 * This flag can then be used for the JMP_Z and JUMP_NZ instructions.
 */
typedef struct flags {
    _Bool z;
} flag_t;


/**
 * This is the signature for an implementation of a bytecode operation.
 *
 * Each operation will receive a pointer to the svm_t, containing the
 * virtual machine reference.
 *
 * Note: Forward-declare the struct so we can use it.
 */
struct svm;
typedef void opcode_implementation(struct svm *in);



/**
 * The Simple Virtual Machine object.
 *
 * All operations relate to this structure, which is allocated
 * via `svm_new` and freed with `svm_free`.
 *
 */
typedef struct svm {
    /**
     * The registers that this virtual machine possesses
     */
    reg_t registers[REGISTER_COUNT];

    /**
     * The flags the CPU contains.
     */
    flag_t flags;

    /**
     * The instruction-pointer.
     */
    unsigned int ip;

    /**
     * The code loaded in the machines RAM, and size of same.
     */
    unsigned char *code;
    unsigned int size;

    /**
     * The user may define a custom error-handler for when
     * register type-errors occur, or there is a division-by-zero
     * error.
     *
     * If set this will be stored here, if not a default implementation
     * will be provided.
     *
     */
    void (*error_handler) (char *msg);

    /**
     * This is a lookup table which maps opcodes to the appropriate handler.
     */
    opcode_implementation *opcodes[256];

    /**
     * This is the stack for the virtual machine.  There are
     * only a small number of entries permitted.
     */
    int stack[1024];

    /**
     * The stack pointer which starts from zero and grows upwards.
     */
    int SP;

    /**
     * State - Shouldn't really be here.
     */
    _Bool running;

} svm_t;



/**
 * Allocate a new virtual machine instance.
 */
svm_t *svm_new(unsigned char *code, unsigned int size);


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
void svm_set_error_handler(svm_t * cpup, void (*fp) (char *msg));


/**
 * This function is called if there is an error in handling
 * a bytecode program - such as a mismatched type, or division by zero.
 *
 */
void svm_default_error_handler(svm_t * cpup, char *msg);


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
