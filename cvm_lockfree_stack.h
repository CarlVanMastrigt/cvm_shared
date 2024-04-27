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

typedef struct cvm_lockfree_stack
{
    alignas(128) atomic_uint_fast64_t head;

    size_t entry_size;
    size_t capacity_exponent;
    char * entry_data;
    uint16_t * next_buffer;
}
cvm_lockfree_stack;

/// declared in another struct for type protection
typedef struct cvm_lockfree_stack_pool
{
    cvm_lockfree_stack available_entries;
}
cvm_lockfree_stack_pool;


void cvm_lockfree_stack_initialise(cvm_lockfree_stack * stack, cvm_lockfree_stack_pool * pool);

void cvm_lockfree_stack_add(cvm_lockfree_stack * stack, void * entry);
void * cvm_lockfree_stack_get(cvm_lockfree_stack * stack);


void cvm_lockfree_stack_pool_initialise(cvm_lockfree_stack_pool * pool, size_t capacity_exponent, size_t entry_size);
void cvm_lockfree_stack_pool_terminate(cvm_lockfree_stack_pool * pool);

void * cvm_lockfree_stack_pool_acquire_entry(cvm_lockfree_stack_pool * pool);
void cvm_lockfree_stack_pool_relinquish_entry(cvm_lockfree_stack_pool * pool, void * entry);

void * cvm_lockfree_stack_pool_call_for_every_entry(cvm_lockfree_stack_pool * pool,void (*func)(void * elem, void * data), void * data);



#endif
