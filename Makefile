#
#  Simple Makefile for the simple virtual machine.
#


all: simple-vm

#
#  Remove our compiled machine, and the sample programs.
#
clean:
	@rm simple-vm *.raw || true


#
#  Compile the virtual machine.
#
simple-vm: simple-vm.c
	@gcc -Wall -Wextra -o simple-vm simple-vm.c -ggdb


#
#  Compile all the examples.
#
compile:
	for i in *.in; do ./compiler $$i >/dev/null  ; done



#
#  Format our source-code.
#
indent:
	find . \( -name '*.c' -o -name '*.h' \) -exec indent  --braces-after-if-line --no-tabs  --k-and-r-style --line-length 90 --indent-level 4 -bli0 \{\} \;
