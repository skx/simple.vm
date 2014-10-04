simple.vm
---------

This repository contains a toy virtual machine intepreter, which reads binary "bytecodes" from a file and executes them.

There is a simple "compiler" which will generate bytecodes from a given
program.  It isn't _really_ a compiler, but it will let you write simple scripts, and will keep track of labels which may be used prior to being defined.


Instructions
------------

There are several implemented instruction-types:

*  Store a string/int into the given register.
*  Mathematical operatoins: add, sub, multiply, divide.
*  Output the contents of a given register. (string/int).
*  A jump (immediate) operation.

The instructions are pretty basic, as this is just a toy, but adding new ones isn't difficult and the available primitives are reasonably useful as-is.


Example
-------

The following program will just endlessly output a string:

     :start
          store #1, "I like loops"
          print_str #1
          goto start

This program first stores the string "`I like loops`" in register 1, then prints that register, before jumping to the start of the program.

To actually compile this program into bytecodes run:

      $ ./compiler simple.in

This will produce an output file full of binary-opcodes in the file `simple.raw`:

      $ od -c simple.raw
      0000000 001 001  \f   I       l   i   k   e       l   o   o   p   s 003
      0000020 001 006  \0  \0
      0000024

Now we can execute that series of opcodes:

      ./simple-vm ./simple.raw

If you wish to debug the execution then run:

      DEBUG=1 ./simple-vm ./simple.raw


TODO
----

More operations - perhaps string-concat, str-str, etc.

Otherwise relax.

Steve
--
