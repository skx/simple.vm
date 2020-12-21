/**
 * embedded.c - Example embedded usage with custom opcode.
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


/**
 * The opcodes we're going to execute.
 */
unsigned char bytecode[] = {
    /* STORE 0x1234 in register 01 */
    0x01,
    0x01,
    0x34,
    0x12,

    /* PRINT register 01 */
    0x02,
    0x01,

    /* CUSTOM OPCODE - Which will call a handler in *this* file! */
    0xCD,

    /* EXIT */
    0x00,

};


/**
 * The handler for the custom opcode
 */
void op_custom(struct svm *svm)
{
    printf("\nCustom Handling Here\n");
    printf("\tOur bytecode is %d bytes long\n", svm->size);

    /* handle the next instruction */
    svm->ip += 1;
}


/**
 * Run the statically-defined bytecode at the head of this script, after
 * defining a custom opcode.
 */
int run_vm()
{

    int size = sizeof(bytecode) / sizeof(bytecode[0]);

    svm_t *cpu = svm_new(bytecode, size);
    if (!cpu)
    {
        printf("Failed to create virtual machine instance.\n");
        return 1;
    }

    /**
     * Allow our our custom handler to be called via opcode 0xCD.
     */
    cpu->opcodes[0xCD] = op_custom;

    /**
     * Run the bytecode.
     */
    svm_run(cpu);

    /**
     * Cleanup.
     */
    svm_free(cpu);
    return 0;
}




/**
 * Simple driver to launch our virtual machine.
 *
 * Given a filename parse/execute the opcodes contained within it.
 *
 */
int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    run_vm();
    return 0;
}
