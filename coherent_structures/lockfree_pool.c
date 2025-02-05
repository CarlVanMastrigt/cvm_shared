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

#include <stdlib.h>
#include <assert.h>

#include "coherent_structures/lockfree_pool.h"

void sol_lockfree_pool_initialise(sol_lockfree_pool * pool, size_t capacity_exponent, size_t entry_size)
{
    size_t i,count;
    assert(capacity_exponent<=24);///requested more capacity than currently possible (consider increasing range, requires altering defines in lockfree_stack header)

    /// the available entries will store the actual pointers rather than duplicates (like other stacks created from this pool)
    atomic_init(&pool->available_entries.head,0);
    pool->available_entries.next_buffer = malloc(sizeof(uint32_t)<<capacity_exponent);
    pool->available_entries.entry_data = malloc(entry_size<<capacity_exponent);
    pool->available_entries.entry_size = entry_size;
    pool->available_entries.capacity_exponent = capacity_exponent;

    count=(size_t)1<<capacity_exponent;

    for(i=0; i<count-1; i++)
    {
        pool->available_entries.next_buffer[i] = (uint32_t)(i + 1);
    }
    pool->available_entries.next_buffer[count-1] = (uint32_t)SOL_LOCKFREE_STACK_INVALID_ENTRY;
}

void sol_lockfree_pool_terminate(sol_lockfree_pool * pool)
{
    free(pool->available_entries.next_buffer);
    free(pool->available_entries.entry_data);
}

void * sol_lockfree_pool_acquire_entry(sol_lockfree_pool * pool)
{
    return sol_lockfree_stack_pull(&pool->available_entries);
}

void sol_lockfree_pool_relinquish_entry(sol_lockfree_pool * pool, void * entry)
{
    sol_lockfree_stack_push(&pool->available_entries, entry);
}

void sol_lockfree_pool_relinquish_entry_index(sol_lockfree_pool* pool, uint32_t entry_index)
{
    sol_lockfree_stack_push_index_range(&pool->available_entries, entry_index, entry_index);
}

void sol_lockfree_pool_relinquish_entry_index_range(sol_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index)
{
    sol_lockfree_stack_push_index_range(&pool->available_entries, first_entry_index, last_entry_index);
}

void sol_lockfree_pool_call_for_every_entry(sol_lockfree_pool * pool,void (*func)(void* entry, void* data), void* data)
{
    size_t i,count;
    count = (size_t)1 << pool->available_entries.capacity_exponent;

    for(i=0;i<count;i++)
    {
        func(pool->available_entries.entry_data + i*pool->available_entries.entry_size, data);
    }
}





