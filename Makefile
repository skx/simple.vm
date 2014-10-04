#
#  Simple Makefile for the simple virtual machine.
#


all: simple-vm


clean:
	@rm simple-vm *.raw || true

simple-vm: simple-vm.c
	@gcc -Wall -Wextra -o simple-vm simple-vm.c -ggdb

test: simple-vm
	@./make-prog > prog.raw || true
	@./simple-vm ./prog.raw    || true

