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

#include "cvm_shared.h"

void cvm_lockfree_pool_initialise(cvm_lockfree_pool * pool, size_t capacity_exponent, size_t entry_size)
{
    size_t i,count;
    assert(capacity_exponent<=16);///requested more capacity than possible (consider switching next to be a u32)

    /// the available entries will store the actual pointers rather than duplicates (like other stacks created from this pool)
    atomic_init(&pool->available_entries.head,0);
    pool->available_entries.next_buffer=malloc(sizeof(uint16_t)<<capacity_exponent);
    pool->available_entries.entry_data=malloc(entry_size<<capacity_exponent);
    pool->available_entries.entry_size=entry_size;
    pool->available_entries.capacity_exponent=capacity_exponent;

    count=(size_t)1<<capacity_exponent;

    for(i=0;i<count-1;i++)
    {
        pool->available_entries.next_buffer[i]=i+1;
    }
    pool->available_entries.next_buffer[count-1]=CVM_INVALID_U16_INDEX;
}

void cvm_lockfree_pool_terminate(cvm_lockfree_pool * pool)
{
    free(pool->available_entries.next_buffer);
    free(pool->available_entries.entry_data);
}



void * cvm_lockfree_pool_acquire_entry(cvm_lockfree_pool * pool)
{
    return cvm_lockfree_stack_get(&pool->available_entries);
}

void cvm_lockfree_pool_relinquish_entry(cvm_lockfree_pool * pool, void * entry)
{
    cvm_lockfree_stack_add(&pool->available_entries,entry);
}


void cvm_lockfree_pool_call_for_every_entry(cvm_lockfree_pool * pool,void (*func)(void * elem, void * data), void * data)
{
    size_t i,count;
    count = (size_t)1 << pool->available_entries.capacity_exponent;

    for(i=0;i<count;i++)
    {
        func(pool->available_entries.entry_data + i*pool->available_entries.entry_size, data);
    }
}





