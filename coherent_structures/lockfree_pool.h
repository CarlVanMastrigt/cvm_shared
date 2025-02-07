/**
Copyright 2024 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <inttypes.h>
#include <stdatomic.h>
#include <assert.h>

/** lockfree_pool
 * a pool is just a stack that has it's own internal backing and starts full
 * it's used for the memory backing of other types (stacks, queues and hoppers)
 * the interplay of pool and stack can be a little complicated to understand, but they are very powerful together*/

// same as stack
#define SOL_LOCKFREE_POOL_CHECK_MASK    ((uint_fast64_t)0xFFFFFFFFFF000000llu)
#define SOL_LOCKFREE_POOL_ENTRY_MASK    ((uint_fast64_t)0x0000000000FFFFFFllu)
#define SOL_LOCKFREE_POOL_INVALID_ENTRY ((uint32_t)0x00FFFFFFu)
#define SOL_LOCKFREE_POOL_CHECK_UNIT    ((uint_fast64_t)0x0000000001000000llu)

/// declared in another struct for type protection
struct sol_lockfree_pool
{
    atomic_uint_fast64_t head;

    size_t entry_size;
    size_t capacity_exponent;
    char* entry_data;
    uint32_t* next_buffer;
};

void sol_lockfree_pool_initialise(struct sol_lockfree_pool* pool, size_t capacity_exponent, size_t entry_size);
void sol_lockfree_pool_terminate(struct sol_lockfree_pool* pool);

void* sol_lockfree_pool_acquire_entry(struct sol_lockfree_pool* pool);
uint32_t sol_lockfree_pool_acquire_entry_index(struct sol_lockfree_pool* pool);

void sol_lockfree_pool_relinquish_entry(struct sol_lockfree_pool* pool, void* entry);
void sol_lockfree_pool_relinquish_entry_index_range(struct sol_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index);

// following mostly for use with a lockfree hopper, iterate will return NULL when there are no remaining elements
void* sol_lockfree_pool_get_entry_pointer(struct sol_lockfree_pool* pool, uint32_t entry_index);
void* sol_lockfree_pool_iterate_range(struct sol_lockfree_pool* pool, uint32_t* entry_index);

void sol_lockfree_pool_call_for_every_entry(struct sol_lockfree_pool* pool, void(*func)(void* entry, void* data), void* data);
