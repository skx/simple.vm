simple.vm
---------

This repository contains a simple virtual machine intepreter, which reads
binary "bytecodes" from a file and executes them.

Because this is only a virtual machine there is no code-generation, if
you wish to execute bytecode you must write it by hand - but to make that
easier there is a perl-script with some generation functions.


Instructions
------------

We have several instruction types:

   1.  Save string/int into register N.

   2.  Math operations which are of the form:
          MATH RESULT_REG, REG1, REG2

   3.  Output operations.

   4.  Misc operations - which just means "system" at the moment.

   5.  Label operations:
        emit a label (with a single character identifier.
        jump to a label.

Example
-------

To compute and display the result of 3 * 40 :

  STORE_INT R1, 3
  STORE_INT R2, 40
  MULT R3, R1,R2
  PRINT_INT R3
   => 120


To constantly output increasing numbers:

    ; set reg0 = 0
    ; set reg1 = 1
    store_int( 0x00, 0 );
    store_int( 0x01, 1 );

    ; emit a lable
    emit_label( "A" );

    ; reg0 = reg0 + reg1
    print chr IADD;
    print chr 0x00;
    print chr 0x00;
    print chr 0x01;

    ; show result
    print_int( 0x00 );

    ; goto A
    jump_label( "A" );


As mentioned there are some perl generators:

    perl ./make-count > count.raw
    ./simple-vm ./count.raw

To enable debugging:

    DEBUG=1 ./simple-vm ./count.raw



TODO: Storing
-------------

* Registers can only be loaded with integers in the range 00-ff

* Strings have to have their lengths set, and are limited to lengths 00-ff.



TODO: Fix Labels
----------------

In an ideal world labels would have string identifiers, and we
could store them as we see them via a linked-list, or hash-table.

However at the moment we just emit labels as instructions, so you
might see:

    0a B   -> Declare label "B"
    ..
    0b B   -> Jump to label "B"

Although this is simple enough it does mean that you can only
jump backwards - because foreward declaration isn't possible.


The only obvious solution here is to parse the code when it is
loaded and look for labels - then build up the lable-jump-table
before executing the code.

Because instructions are different lengths though this becomes
more of a pain that we'd like.
