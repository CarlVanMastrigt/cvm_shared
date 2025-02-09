/**
Copyright 2024,2025 Carl van Mastrigt

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

#include "lockfree/pool.h"

void sol_lockfree_pool_initialise(struct sol_lockfree_pool* pool, size_t capacity_exponent, size_t entry_size)
{
    size_t i,count;
    assert(capacity_exponent<=24);///requested more capacity than currently possible (consider increasing range, see defines above)

    /// the available entries will store the actual pointers rather than duplicates (like other pools created from this pool)
    atomic_init(&pool->head,0);
    pool->next_buffer = malloc(sizeof(uint32_t)<<capacity_exponent);
    pool->entry_data = malloc(entry_size<<capacity_exponent);
    pool->entry_size = entry_size;
    pool->capacity_exponent = capacity_exponent;

    count=(size_t)1<<capacity_exponent;

    for(i=0; i<count-1; i++)
    {
        pool->next_buffer[i] = (uint32_t)(i + 1);
    }
    pool->next_buffer[count-1] = SOL_LOCKFREE_POOL_INVALID_ENTRY;
}

void sol_lockfree_pool_terminate(struct sol_lockfree_pool* pool)
{
    free(pool->next_buffer);
    free(pool->entry_data);
}

uint32_t sol_lockfree_pool_acquire_entry_index(struct sol_lockfree_pool* pool)
{
    uint_fast64_t entry_index, current_head, replacement_head;

    current_head = atomic_load_explicit(&pool->head, memory_order_acquire);

    do
    {
        entry_index = current_head & SOL_LOCKFREE_POOL_ENTRY_MASK;
        /// if there are no more entries in this list then return NULL
        if(entry_index == (uint_fast64_t)SOL_LOCKFREE_POOL_INVALID_ENTRY)
        {
            return SOL_LOCKFREE_POOL_INVALID_ENTRY;
        }

        replacement_head = ((current_head & SOL_LOCKFREE_POOL_CHECK_MASK) + SOL_LOCKFREE_POOL_CHECK_UNIT) | (uint_fast64_t)(pool->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&pool->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed

    assert(entry_index < ((uint_fast64_t)1 << pool->capacity_exponent));

    return entry_index;
}

void * sol_lockfree_pool_acquire_entry(struct sol_lockfree_pool* pool)
{
    uint32_t entry_index = sol_lockfree_pool_acquire_entry_index(pool);

    return sol_lockfree_pool_get_entry_pointer(pool, entry_index);
}


void sol_lockfree_pool_relinquish_entry(struct sol_lockfree_pool* pool, void * entry)
{
    uint32_t entry_index;

    assert((char*)entry >= pool->entry_data);
    assert((char*)entry < pool->entry_data + (pool->entry_size << pool->capacity_exponent));

    entry_index = (uint32_t) (((char*)entry - pool->entry_data) / pool->entry_size);

    sol_lockfree_pool_relinquish_entry_index_range(pool, entry_index, entry_index);
}

void sol_lockfree_pool_relinquish_entry_index_range(struct sol_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index)
{
    uint_fast64_t current_head, replacement_head;

    if(first_entry_index == SOL_LOCKFREE_POOL_INVALID_ENTRY)
    {
        assert(last_entry_index == SOL_LOCKFREE_POOL_INVALID_ENTRY);
        return;
    }

    assert((uint_fast64_t)first_entry_index < ((uint_fast64_t)1 << pool->capacity_exponent));
    assert((uint_fast64_t)last_entry_index  < ((uint_fast64_t)1 << pool->capacity_exponent));

    current_head = atomic_load_explicit(&pool->head, memory_order_relaxed);

    do
    {
        pool->next_buffer[last_entry_index] = (uint32_t)(current_head & SOL_LOCKFREE_POOL_ENTRY_MASK);
        replacement_head = ((current_head & SOL_LOCKFREE_POOL_CHECK_MASK) + SOL_LOCKFREE_POOL_CHECK_UNIT) | first_entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&pool->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));
}



void* sol_lockfree_pool_get_entry_pointer(struct sol_lockfree_pool* pool, uint32_t entry_index)
{
    assert(entry_index < ((uint32_t)1 << pool->capacity_exponent) || entry_index == SOL_LOCKFREE_POOL_INVALID_ENTRY);

    if((uint_fast64_t)entry_index == SOL_LOCKFREE_POOL_INVALID_ENTRY)
    {
        return NULL;
    }
    else
    {
        return pool->entry_data + entry_index * pool->entry_size;
    }
}

void* sol_lockfree_pool_iterate(struct sol_lockfree_pool* pool, uint32_t* entry_index)
{
    uint32_t next_entry_index;

    next_entry_index = pool->next_buffer[*entry_index];

    if(next_entry_index == SOL_LOCKFREE_POOL_INVALID_ENTRY)
    {
        return NULL;
    }
    else
    {
        *entry_index = next_entry_index;
        return pool->entry_data + next_entry_index * pool->entry_size;
    }
}


void sol_lockfree_pool_call_for_every_entry(struct sol_lockfree_pool* pool, void(*func)(void* entry, void* data), void* data)
{
    size_t i,count;
    count = (size_t)1 << pool->capacity_exponent;

    for(i=0;i<count;i++)
    {
        func(pool->entry_data + i * pool->entry_size, data);
    }
}





