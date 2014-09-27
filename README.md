A trivial register-based virtual machine with only the most basic
operations supported.

TODO
----

1.  Add flags to allow tests.
2.  Add loops/control-flow


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



TODO
----

Caution labels should be given strings, but string-types have been
defined with lenths, which means we'd need to have:

  DEFINE_LABEL 3 "foo"
  JMP_LABEL  3 "foo"

Instead I guess we must say "255 labels per program"

Store labels in a linked-list of:

    LABEL[255] = eip;

