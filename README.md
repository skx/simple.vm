simple.vm
---------

This repository contains a toy virtual machine intepreter, which reads
binary "bytecodes" from a file and executes them.

Because this is only a virtual machine there is no code-generation, if
you wish to execute bytecode you must write it by hand - but to make that
easier there is a perl-script with some generation functions.


Instructions
------------

We have several instruction types:

   1.  Save string/int into register N.

   2.  Output operations.

   3.  A jump operation


Example
-------

The following program will just output a string:

    store #1, "I like loops"
    print_str #1
    jump 0

This program first stores the string "`I like loops`" in register 1,
then prints that register, before jumping to the start of the program.

To enable the debugging of execution:

    DEBUG=1 ./simple-vm ./program.raw


