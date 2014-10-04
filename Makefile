#
#  Simple Makefile for the simple virtual machine.
#


#
#  Common definitions.
#
CC=gcc
LINKER=$(CC) -o
CFLAGS+=-pedantic -std=c99 -Wall -Wextra -ggdb




SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:%.c=%.o)



all: simple-vm

simple-vm: $(OBJECTS)
	$(LINKER) $@  $(OBJECTS) $(CFLAGS)


#
#  Remove our compiled machine, and the sample programs.
#
clean:
	@rm simple-vm *.raw *.o || true



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
