/**
 * simple-vm-opcodes.h - Definitions of our opcode-handlers.
 *
 * Copyright (c) 2014 by Steve Kemp.  All rights reserved.
 *
 **
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991, or (at your
 * option) any later version.
 *
 * On Debian GNU/Linux systems, the complete text of version 2 of the GNU
 * General Public License can be found in `/usr/share/common-licenses/GPL-2'
 *
 **
 *
 */


#ifndef SIMPLE_VM_OPCODES_H
#define SIMPLE_VM_OPCODES_H 1


_Bool op_exit(void *in);
_Bool op_nop(void *in);

_Bool op_int_store(void *in);
_Bool op_int_print(void *in);
_Bool op_int_tostring(void *in);

_Bool op_string_store(void *in);
_Bool op_string_print(void *in);
_Bool op_string_concat(void *in);
_Bool op_string_system(void *in);
_Bool op_string_toint(void *in);

_Bool op_jump_to(void *in);
_Bool op_jump_z(void *in);
_Bool op_jump_nz(void *in);

_Bool op_xor(void *in);
_Bool op_add(void *in);
_Bool op_sub(void *in);
_Bool op_mul(void *in);
_Bool op_div(void *in);
_Bool op_inc(void *in);
_Bool op_dec(void *in);

_Bool op_cmp_reg(void *in);
_Bool op_cmp_immediate(void *in);

_Bool op_load_from_ram(void *in);
_Bool op_store_in_ram(void *in);


/**
 * Initi function.
 */
void opcode_init(svm_t * cpu);

#endif                          /* SIMPLE_VM_OPCODES_H */
