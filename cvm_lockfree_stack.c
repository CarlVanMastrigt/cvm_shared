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


void cvm_lockfree_stack_initialise(cvm_lockfree_stack * stack, cvm_lockfree_stack_pool * pool)
{
    atomic_init(&stack->head,((uint_fast64_t)CVM_INVALID_U16_INDEX));
    stack->next_buffer=pool->available_entries.next_buffer;
    stack->entry_data=pool->available_entries.entry_data;
    stack->entry_size=pool->available_entries.entry_size;
    stack->capacity_exponent=pool->available_entries.capacity_exponent;
}

void cvm_lockfree_stack_add(cvm_lockfree_stack * stack, void * entry)
{
    uint_fast64_t entry_index,current_head,replacement_head;
    size_t entry_offset;

    assert((char*)entry >= stack->entry_data);

    entry_offset = (char*)entry - stack->entry_data;
    entry_index = (uint_fast64_t) (entry_offset / stack->entry_size);

    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));

    current_head=atomic_load_explicit(&stack->head, memory_order_relaxed);
    do
    {
        stack->next_buffer[entry_index] = (uint16_t)(current_head & CVM_U16_U64_LOWER_MASK);
        replacement_head=((current_head & CVM_U16_U64_UPPER_MASK) + CVM_U16_U64_UPPER_UNIT) | entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));
}

void * cvm_lockfree_stack_get(cvm_lockfree_stack * stack)
{
    uint_fast64_t entry_index,current_head,replacement_head;

    current_head=atomic_load_explicit(&stack->head, memory_order_acquire);
    do
    {
        entry_index = current_head & CVM_U16_U64_LOWER_MASK;
        /// if there are no more entries in this list then return NULL
        if(entry_index == CVM_INVALID_U16_INDEX)
        {
            return NULL;
        }

        replacement_head = ((current_head & CVM_U16_U64_UPPER_MASK) + CVM_U16_U64_UPPER_UNIT) | (uint_fast64_t)(stack->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed


    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));

    return stack->entry_data + entry_index*stack->entry_size;
}


void cvm_lockfree_stack_pool_initialise(cvm_lockfree_stack_pool * pool, size_t capacity_exponent, size_t entry_size)
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

void cvm_lockfree_stack_pool_terminate(cvm_lockfree_stack_pool * pool)
{
    free(pool->available_entries.next_buffer);
    free(pool->available_entries.entry_data);
}



void * cvm_lockfree_stack_pool_acquire_entry(cvm_lockfree_stack_pool * pool)
{
    return cvm_lockfree_stack_get(&pool->available_entries);
}

void cvm_lockfree_stack_pool_relinquish_entry(cvm_lockfree_stack_pool * pool, void * entry)
{
    cvm_lockfree_stack_add(&pool->available_entries,entry);
}


void * cvm_lockfree_stack_pool_call_for_every_entry(cvm_lockfree_stack_pool * pool,void (*func)(void * elem, void * data), void * data)
{
    size_t i,count;
    count = (size_t)1 << pool->available_entries.capacity_exponent;

    for(i=0;i<count;i++)
    {
        func(pool->available_entries.entry_data + i*pool->available_entries.entry_size, data);
    }
}




