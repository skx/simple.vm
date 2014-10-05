/**
 * main.c - Example driver-file for simple virtual machine.
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
#include <sys/types.h>
#include <sys/stat.h>


#include "simple-vm.h"



void error(char *msg)
{
    fprintf(stderr, "ERROR running script - %s\n", msg);
    exit(1);
}



int run_file(const char *filename, int dump_registers)
{
    struct stat sb;

    if (stat(filename, &sb) != 0)
    {
        printf("Failed to read file: %s\n", filename);
        return 1;
    }

    int size = sb.st_size;

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        printf("Failed to open program-file %s\n", filename);
        return 1;
    }

    unsigned char *code = malloc(size);
    memset(code, '\0', size);

    if (!code)
    {
        printf("Failed to allocate RAM for program-file %s\n", filename);
        fclose(fp);
        return 1;
    }

    fread(code, 1, size, fp);
    fclose(fp);

    svm_t *cpu = svm_new(code, size);
    if (!cpu)
    {
        printf("Failed to create virtual machine instance.\n");
        return 1;
    }

    /**
     * Set the error-handler.
     */
    svm_set_error_handler(cpu, &error);


    /**
     * Run the bytecode.
     */
    svm_run(cpu);


    /**
     * Dump?
     */
    if (dump_registers)
        svm_dump_registers(cpu);


    /**
     * Cleanup.
     */
    svm_free(cpu);
    free(code);
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
    int dump_registers = 0;

    if (argc < 2)
    {
        printf("Usage: %s input-file\n", argv[0]);
        return 0;
    }

    if (getenv("DEBUG") != NULL)
        dump_registers = 1;

    return (run_file(argv[1], dump_registers));

}
