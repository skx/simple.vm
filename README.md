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

The following program will just endlessly output a string:

     :start
          store #1, "I like loops"
          print_str #1
          goto start

This program first stores the string "`I like loops`" in register 1,
then prints that register, before jumping to the start of the program.

To actually compile this program into bytecodes run:

      $ ./compiler simple.in
      1. Compiling simple.in into simple.raw
      2. Post-compile jump-fixups for simple.raw
          We must update offset 18 with the destination of the label: start

This will produce the opcodes to the file `simple.raw`:

      $ od -c simple.raw
      0000000 001 001  \f   I       l   i   k   e       l   o   o   p   s 003
      0000020 001 006  \0  \0
      0000024

Now we can execute that series of opcodes:

      DEBUG=1 ./simple-vm ./simple.raw



TODO
----

* Reinstate the mathematical operations.
* Add some form of conditional(s).
* Add system(RegN), or similar.
