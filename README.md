simple.vm
---------

This repository contains a toy virtual machine intepreter, which reads
binary "bytecodes" from a file and executes them.

There is a simple "compiler" which will generate bytecodes from a given
program - it isn't quite a compiler, but it will let you write simple scripts.


Instructions
------------

We have several instruction types:

   1.  Store a string/int into the given register.

   2.  Output the contents of a given register. (string/int).

   3.  A jump operation.

There is a compiler which takes care of converting from labels to operations.


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



TODO
----

* Reinstate the mathematical operations.
* Add some form of conditional(s).
* Add system(RegN), or similar.
