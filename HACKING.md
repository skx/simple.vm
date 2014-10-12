
Overview
========

This document briefly describes the internal structure of this repository.

The implementation broadly falls into two camps:

* The implementation of the virtual machine itself.
* The implementation of the opcodes.


Virtual Machine Implementation
------------------------------

The virtual machine is encapsulated in the `svm_t` structure, which
contains:

* The virtual-machine registers.
* The virtual-machine flags.
* The code to be executed.
* The stack.

All the implementation of the virtual machine lives in the `simple-vm.c` file,
with the public parts exposed to `simple-vm.h`.

Opcode Implementation
---------------------

The opcodes are all implemented in the file `simple-vm-opcodes.c`, with the public parts exposed via `simple-vm-opcodes.h`.

There are several utility/helper methods which are deliberately not exposed as these are considered internal details.  For example:

* Reading a byte from the current instruction-pointer - incrementing it too.
* Building up an address from two distinct bytes.
* Fetching strings from beneath the instruction-pointer.

Each opcode is implemented by a single function which has the following signature:

    _Bool op_name(svm_t * svm)

The sole argument is a pointer to the virtual machine instance, as you'd expect, but the return value requires some consideration.

The return type of the function declares whether the handler has updated the instruction-pointer:

 * If the function returns `false` then the default processing should occur.
     * The virtual machine should continue to increment the IP and handle the next instruction.
     * The function did nothing sneaky.
 * If the function returns `true` then the opcode itself has fiddled with the IP
     * The virtual machine should not increment the IP, and should instead just keep executing from the updated location.
     * The function did something sneaky.

Examples of functions which update the IP internally include the expected (RET, JUMP, etc) but also the unexpected (STRING_STORE).  String handling is a little atypical in this virtual machine because string-data is included directly in the control-flow.  This means skipping past the inline data requires updating the instruction-pointer.


Compiler
--------

The compiler is really nothing more than a series of regular expressions, with a small amount of extra knowledge required to keep track of instruction-lengths.

By keeping track of instruction-lengths jump-targets can be updated post-compilation.
