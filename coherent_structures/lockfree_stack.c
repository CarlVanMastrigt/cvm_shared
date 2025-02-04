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

#include "coherent_structures/lockfree_stack.h"
#include <assert.h>

void cvm_lockfree_stack_initialise(cvm_lockfree_stack * stack, cvm_lockfree_pool * pool)
{
    atomic_init(&stack->head,CVM_LOCKFREE_STACK_INVALID_ENTRY);
    stack->next_buffer = pool->available_entries.next_buffer;
    stack->entry_data = pool->available_entries.entry_data;
    stack->entry_size = pool->available_entries.entry_size;
    stack->capacity_exponent = pool->available_entries.capacity_exponent;
}

void cvm_lockfree_stack_terminate(cvm_lockfree_stack * stack)
{
    assert((atomic_load_explicit(&stack->head, memory_order_relaxed) & CVM_LOCKFREE_STACK_ENTRY_MASK) == CVM_LOCKFREE_STACK_INVALID_ENTRY);/// stack should be empty upon termination
}


void cvm_lockfree_stack_push_index_range(cvm_lockfree_stack * stack, uint32_t first_entry_index, uint32_t last_entry_index)
{
    uint_fast64_t current_head, replacement_head;

    assert(first_entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));
    assert(last_entry_index  < ((uint_fast64_t)1 << stack->capacity_exponent));

    current_head = atomic_load_explicit(&stack->head, memory_order_relaxed);
    do
    {
        stack->next_buffer[last_entry_index] = (uint32_t)(current_head & CVM_LOCKFREE_STACK_ENTRY_MASK);
        replacement_head = ((current_head & CVM_LOCKFREE_STACK_CHECK_MASK) + CVM_LOCKFREE_STACK_CHECK_UNIT) | first_entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));
}

void cvm_lockfree_stack_push_index(cvm_lockfree_stack * stack, uint32_t entry_index)
{
    cvm_lockfree_stack_push_index_range(stack, entry_index, entry_index);
}

uint32_t cvm_lockfree_stack_pull_index(cvm_lockfree_stack * stack)
{
    uint_fast64_t entry_index,current_head,replacement_head;

    current_head=atomic_load_explicit(&stack->head, memory_order_acquire);
    do
    {
        entry_index = current_head & CVM_LOCKFREE_STACK_ENTRY_MASK;
        /// if there are no more entries in this list then return NULL / CVM_LOCKFREE_STACK_INVALID_ENTRY_U32
        if(entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY)
        {
            return (uint32_t)CVM_LOCKFREE_STACK_INVALID_ENTRY;
        }

        replacement_head = ((current_head & CVM_LOCKFREE_STACK_CHECK_MASK) + CVM_LOCKFREE_STACK_CHECK_UNIT) | (uint_fast64_t)(stack->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed

    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));
    assert(entry_index <= UINT32_MAX);

    return entry_index;
}


void cvm_lockfree_stack_push_range(cvm_lockfree_stack* stack, void* first_entry, void* last_entry)
{
    uint32_t first_entry_index, last_entry_index;

    assert((char*)first_entry >= stack->entry_data);
    assert((char*)last_entry  >= stack->entry_data);

    assert((char*)first_entry < stack->entry_data + (stack->entry_size << stack->capacity_exponent));
    assert((char*)last_entry  < stack->entry_data + (stack->entry_size << stack->capacity_exponent));

    first_entry_index = (uint32_t) (((char*)first_entry - stack->entry_data) / (ptrdiff_t)stack->entry_size);
    last_entry_index  = (uint32_t) (((char*)last_entry  - stack->entry_data) / (ptrdiff_t)stack->entry_size);

    cvm_lockfree_stack_push_index_range(stack, first_entry_index, last_entry_index);
}

void cvm_lockfree_stack_push(cvm_lockfree_stack* stack, void * entry)
{
    uint32_t entry_index;

    assert((char*)entry >= stack->entry_data);
    assert((char*)entry < stack->entry_data + (stack->entry_size << stack->capacity_exponent));

    entry_index = (uint32_t) (((char*)entry - stack->entry_data) / (ptrdiff_t)stack->entry_size);

    cvm_lockfree_stack_push_index_range(stack, entry_index, entry_index);
}

void * cvm_lockfree_stack_pull(cvm_lockfree_stack* stack)
{
    uint32_t entry_index;

    entry_index = cvm_lockfree_stack_pull_index(stack);

    if(entry_index == (uint32_t)CVM_LOCKFREE_STACK_INVALID_ENTRY)
    {
        return NULL;;
    }

    return stack->entry_data + entry_index * stack->entry_size;
}


