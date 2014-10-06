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
 * This is the signature for an implementation of a bytecode operation.
 *
 * Each operation will receive a pointer to the svm_t,
 * referring to the virtual machine, however we cannot
 * use that type here because we want the list of operations
 * to be associated with the svm_t which has not yet been
 * declared.
 *
 * (i.e.  We really want a forward declaration here, but we're C not C++.)
 *
 */
typedef _Bool opcode_implementation(void *);



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
     * This is a lookup table which maps opcodes to the handlers
     * for them.
     */
    opcode_implementation *opcodes[255];

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
