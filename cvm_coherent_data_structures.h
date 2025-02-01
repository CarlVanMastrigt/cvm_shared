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

#ifndef CVM_COHERENT_DATA_STRUCTURES_H
#define CVM_COHERENT_DATA_STRUCTURES_H

/// shared as a base between stacks and pools
typedef struct cvm_lockfree_pool cvm_lockfree_pool;

#include "cvm_lockfree_stack.h"
#include "cvm_lockfree_hopper.h"
#include "cvm_coherent_queue.h"

/// declared in another struct for type protection
struct cvm_lockfree_pool
{
    cvm_lockfree_stack available_entries;
};

void cvm_lockfree_pool_initialise(cvm_lockfree_pool * pool, size_t capacity_exponent, size_t entry_size);
void cvm_lockfree_pool_terminate(cvm_lockfree_pool * pool);

void* cvm_lockfree_pool_acquire_entry(cvm_lockfree_pool * pool);
void cvm_lockfree_pool_relinquish_entry(cvm_lockfree_pool * pool, void * entry);

void cvm_lockfree_pool_call_for_every_entry(cvm_lockfree_pool * pool,void (*func)(void * elem, void * data), void * data);



#endif

