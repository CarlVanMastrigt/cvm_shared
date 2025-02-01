/**
Copyright 2024 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_LOCKFREE_STACK_H
#define CVM_LOCKFREE_STACK_H

#define CVM_LOCKFREE_STACK_CHECK_MASK ((uint_fast64_t)0xFFFFFFFFFFFF0000llu)
#define CVM_LOCKFREE_STACK_ENTRY_MASK ((uint_fast64_t)0x000000000000FFFFllu)

#define CVM_LOCKFREE_STACK_INVALID_ENTRY ((uint_fast64_t)0x000000000000FFFFllu)

#define CVM_LOCKFREE_STACK_CHECK_UNIT ((uint_fast64_t)0x0000000000010000llu)

typedef struct cvm_lockfree_stack
{
    _Alignas(128) atomic_uint_fast64_t head;

    size_t entry_size;
    size_t capacity_exponent;
    char * entry_data;
    uint16_t * next_buffer;
}
cvm_lockfree_stack;

/// a stack uses a pool for backing
void cvm_lockfree_stack_initialise(cvm_lockfree_stack * stack, cvm_lockfree_pool * pool);
void cvm_lockfree_stack_terminate(cvm_lockfree_stack * stack);

void cvm_lockfree_stack_push(cvm_lockfree_stack * stack, void * entry);
void * cvm_lockfree_stack_pull(cvm_lockfree_stack * stack);


#endif
