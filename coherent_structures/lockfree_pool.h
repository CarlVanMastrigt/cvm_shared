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

#include "coherent_structures.h"

#ifndef CVM_LOCKFREE_POOL_H
#define CVM_LOCKFREE_POOL_H

/** lockfree_pool
 * a pool is just a stack that has it's own internal backing and starts full
 * it's used for the memory backing of other types (stacks, queues and hoppers)
 * the interplay of pool and stack can be a little complicated to understand, but they are very powerful together*/


/// declared in another struct for type protection
struct cvm_lockfree_pool
{
    struct cvm_lockfree_stack available_entries;
};

void cvm_lockfree_pool_initialise(cvm_lockfree_pool* pool, size_t capacity_exponent, size_t entry_size);
void cvm_lockfree_pool_terminate(cvm_lockfree_pool* pool);

void* cvm_lockfree_pool_acquire_entry(cvm_lockfree_pool* pool);
void cvm_lockfree_pool_relinquish_entry(cvm_lockfree_pool* pool, void* entry);

void cvm_lockfree_pool_relinquish_entry_index(cvm_lockfree_pool* pool, uint32_t entry_index);
void cvm_lockfree_pool_relinquish_entry_index_range(cvm_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index);

void cvm_lockfree_pool_call_for_every_entry(cvm_lockfree_pool* pool,void (*func)(void* entry, void* data), void* data);



#endif

