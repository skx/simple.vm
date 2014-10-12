#
#  Simple Makefile for the simple virtual machine.
#


#
#  Common definitions.
#
CC=gcc
LINKER=$(CC) -o
CFLAGS+=-O2 -W -Wall -Wextra -pedantic -std=c11



#
#  The default targets
#
all: simple-vm embedded

#
#  The sample driver.
#
simple-vm: main.o simple-vm.o simple-vm-opcodes.o
	$(LINKER) $@  $(OBJECTS) $(CFLAGS) main.o simple-vm.o simple-vm-opcodes.o


#
#  A program that contains an embedded virtual machine and allows
# that machine to call into the application via a custom opcode 0xCD.
#
embedded: embedded.o simple-vm.o simple-vm-opcodes.o
	$(LINKER) $@  $(OBJECTS) $(CFLAGS) simple-vm.o embedded.o simple-vm-opcodes.o


#
#  Remove our compiled machine, and the sample programs.
#
clean:
	@rm simple-vm embedded *.raw *.o || true



#
#  Compile all the examples.
#
compile:
	for i in examples/*.in; do ./compiler $$i >/dev/null  ; done



#
#  Format our source-code.
#
indent:
	find . \( -name '*.c' -o -name '*.h' \) -exec indent  --braces-after-if-line --no-tabs  --k-and-r-style --line-length 90 --indent-level 4 -bli0 \{\} \;
	perltidy compiler
